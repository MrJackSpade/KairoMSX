// KairoMSX's PostProcessor (KairoPostProcessor.hh). rotateFrames() is openMSX's own frame
// selection (video/PostProcessor.cc) without the OpenGL uploads; paint() copies the chosen frame
// into the KairoOutputSurface.

#include "PostProcessor.hh"

#include "AviRecorder.hh"
#include "CliComm.hh"
#include "CommandException.hh"
#include "Deflicker.hh"
#include "DeinterlacedFrame.hh"
#include "Display.hh"
#include "DoubledFrame.hh"
#include "KairoOutputSurface.hh"
#include "MSXMotherBoard.hh"
#include "PNG.hh"
#include "RawFrame.hh"
#include "RenderSettings.hh"
#include "SuperImposedFrame.hh"

#include "MemBuffer.hh"
#include "aligned.hh"
#include "checked_cast.hh"
#include "inplace_buffer.hh"
#include "narrow.hh"
#include "stl.hh"
#include "xrange.hh"

#include <algorithm>
#include <cassert>

namespace openmsx {

PostProcessor::PostProcessor(
	MSXMotherBoard& motherBoard_, Display& display_,
	OutputSurface& /*screen*/, const std::string& videoSource,
	unsigned maxWidth_, unsigned height_, bool canDoInterlace_)
	: VideoLayer(motherBoard_, videoSource)
	, display(display_)
	, renderSettings(display_.getRenderSettings())
	, maxWidth(maxWidth_)
	, height(height_)
	, canDoInterlace(canDoInterlace_)
{
	if (canDoInterlace) {
		deinterlacedFrame = std::make_unique<DeinterlacedFrame>();
		interlacedFrame   = std::make_unique<DoubledFrame>();
		deflicker = std::make_unique<Deflicker>(lastFrames);
		superImposedFrame = std::make_unique<SuperImposedFrame>();
	}
}

PostProcessor::~PostProcessor()
{
	if (recorder) {
		getCliComm().printWarning(
			"Video recording stopped, because you "
			"changed machine or changed a video setting "
			"during recording.");
		recorder->stop();
	}
}

CliComm& PostProcessor::getCliComm()
{
	return display.getCliComm();
}

unsigned PostProcessor::getLineWidth(
	FrameSource* frame, unsigned y, unsigned step)
{
	return max_value(xrange(step), [&](auto i) { return frame->getLineWidth(y + i); });
}

void PostProcessor::paint(OutputSurface& output)
{
	if (!paintFrame) return;
	// Lines narrower than the widest are widened to it, so nothing is lost.
	unsigned width = getLineWidth(paintFrame, 0, paintFrame->getHeight());
	unsigned lines = paintFrame->getHeight();
	auto picture = checked_cast<KairoOutputSurface&>(output).startPicture(width, lines);
	for (auto y : xrange(lines)) {
		auto row = picture.subspan(size_t(y) * width, width);
		auto line = paintFrame->getLine(narrow<int>(y), row);
		if (line.data() != row.data()) std::ranges::copy(line, row.begin());
	}
}

using WorkBuffer = std::vector<MemBuffer<FrameSource::Pixel, SSE_ALIGNMENT>>;
static void getScaledFrame(const FrameSource& paintFrame,
                           std::span<const FrameSource::Pixel*> lines,
                           WorkBuffer& workBuffer)
{
	auto height = narrow<unsigned>(lines.size());
	unsigned width = (height == 240) ? 320 : 640;
	const FrameSource::Pixel* linePtr = nullptr;
	FrameSource::Pixel* work = nullptr;
	for (auto i : xrange(height)) {
		if (linePtr == work) {
			// If work buffer was used in previous iteration,
			// then allocate a new one.
			work = workBuffer.emplace_back(width).data();
		}
		if (height == 240) {
			auto line = paintFrame.getLinePtr320_240(i, std::span<uint32_t, 320>{work, 320});
			linePtr = line.data();
		} else {
			assert (height == 480);
			auto line = paintFrame.getLinePtr640_480(i, std::span<uint32_t, 640>{work, 640});
			linePtr = line.data();
		}
		lines[i] = linePtr;
	}
}

void PostProcessor::takeRawScreenShot(std::optional<unsigned> desiredHeight, const std::string& filename)
{
	if (!paintFrame) {
		throw CommandException("TODO");
	}

	const unsigned targetHeight = desiredHeight ? *desiredHeight : [this] {
		auto maxLineWidth = getLineWidth(paintFrame, 0, paintFrame->getHeight());
		if (maxLineWidth == 320) return 240;
		// in all other cases (there could be other than 640), we go for 640x480
		return 480;
	}();

	inplace_buffer<const FrameSource::Pixel*, 480> lines(uninitialized_tag{}, targetHeight);
	WorkBuffer workBuffer;
	getScaledFrame(*paintFrame, lines, workBuffer);
	unsigned width = (targetHeight == 240) ? 320 : 640;
	PNG::saveRGBA(width, lines, filename);
}

std::unique_ptr<RawFrame> PostProcessor::rotateFrames(
	std::unique_ptr<RawFrame> finishedFrame, EmuTime time)
{
	// Figure out how many past frames we want to use.
	int numRequired = 1;
	bool doDeinterlace = false;
	bool doInterlace   = false;
	bool doDeflicker   = false;
	auto currType = finishedFrame->getField();
	if (canDoInterlace) {
		if (currType != FrameSource::FieldType::NONINTERLACED) {
			if (renderSettings.getDeinterlace()) {
				doDeinterlace = true;
				numRequired = 2;
			} else {
				doInterlace = true;
			}
		} else if (renderSettings.getDeflicker()) {
			doDeflicker = true;
			numRequired = 4;
		}
	}

	// Which frame can be returned (recycled) to caller. Prefer to return
	// the youngest frame to improve cache locality.
	int recycleIdx = (lastFramesCount < numRequired)
		? lastFramesCount++  // store one more
		: (numRequired - 1); // youngest that's no longer needed
	assert(recycleIdx < 4);
	auto recycleFrame = std::move(lastFrames[recycleIdx]); // might be nullptr

	// Insert new frame in front of lastFrames[], shift older frames
	std::move_backward(&lastFrames[0], &lastFrames[recycleIdx],
	                   &lastFrames[recycleIdx + 1]);
	lastFrames[0] = std::move(finishedFrame);

	// Are enough frames available?
	if (lastFramesCount >= numRequired) {
		// Only the last 'numRequired' are kept up to date.
		lastFramesCount = numRequired;
	} else {
		// Not enough past frames, fall back to 'regular' rendering.
		doDeinterlace = false;
		doInterlace   = false;
		doDeflicker   = false;
	}

	// Setup the to-be-painted frame
	if (doDeinterlace) {
		if (currType == FrameSource::FieldType::ODD) {
			deinterlacedFrame->init(lastFrames[1].get(), lastFrames[0].get());
		} else {
			deinterlacedFrame->init(lastFrames[0].get(), lastFrames[1].get());
		}
		paintFrame = deinterlacedFrame.get();
	} else if (doInterlace) {
		interlacedFrame->init(
			lastFrames[0].get(),
			(currType == FrameSource::FieldType::ODD) ? 1 : 0);
		paintFrame = interlacedFrame.get();
	} else if (doDeflicker) {
		deflicker->init();
		paintFrame = deflicker.get();
	} else {
		paintFrame = lastFrames[0].get();
	}
	if (superImposeVdpFrame) {
		superImposedFrame->init(paintFrame, superImposeVdpFrame);
		paintFrame = superImposedFrame.get();
	}

	// Possibly record this frame
	if (recorder && needRecord()) {
		try {
			recorder->addImage(paintFrame, time);
		} catch (MSXException& e) {
			getCliComm().printWarning(
				"Recording stopped with error: ",
				e.getMessage());
			recorder->stop();
			assert(!recorder);
		}
	}

	// Return recycled frame to the caller
	if (canDoInterlace) {
		if (!recycleFrame) [[unlikely]] {
			recycleFrame = std::make_unique<RawFrame>(maxWidth, height);
		}
		return recycleFrame;
	} else {
		return std::move(lastFrames[0]);
	}
}

} // namespace openmsx
