// openMSX's PostProcessor for KairoMSX: the video layer that turns the VDP's frames into the
// picture, without OpenGL. It chooses the frame to show as openMSX's own does (interlacing,
// deinterlacing, deflicker, superimposing a second VDP, recording) and paints it at its native
// size into the host's KairoOutputSurface; the host applies any scaling and filters. The GL
// display effects (scalers, noise, glow, 3D deform, black frame interleaving) are not drawn, and
// neither is a laserdisc video frame, as KairoMSX builds no laserdisc player.
// video/PostProcessor.hh includes this in KairoMSX's build (tools/local_patches.py), so the rasterizers use it unchanged.

#ifndef KAIRO_POSTPROCESSOR_HH
#define KAIRO_POSTPROCESSOR_HH

#include "RenderSettings.hh"
#include "VideoLayer.hh"

#include "EmuTime.hh"

#include <array>
#include <memory>

namespace openmsx {

class AviRecorder;
class CliComm;
class Deflicker;
class DeinterlacedFrame;
class Display;
class DoubledFrame;
class FrameSource;
class MSXMotherBoard;
class RawFrame;
class SuperImposedFrame;

class PostProcessor final : public VideoLayer
{
public:
	PostProcessor(
		MSXMotherBoard& motherBoard, Display& display,
		OutputSurface& screen, const std::string& videoSource,
		unsigned maxWidth, unsigned height, bool canDoInterlace);
	~PostProcessor() override;

	// Layer interface:
	void paint(OutputSurface& output) override;

	/** Takes the frame that has just been finished and returns one to build the next in. */
	[[nodiscard]] std::unique_ptr<RawFrame> rotateFrames(
		std::unique_ptr<RawFrame> finishedFrame, EmuTime time);

	void setSuperimposeVideoFrame(const RawFrame* videoSource) {
		superImposeVideoFrame = videoSource;
	}
	void setSuperimposeVdpFrame(const FrameSource* vdpSource) {
		superImposeVdpFrame = vdpSource;
	}

	void setRecorder(AviRecorder* recorder_) { recorder = recorder_; }
	[[nodiscard]] bool isRecording() const { return recorder != nullptr; }

	[[nodiscard]] FrameSource* getPaintFrame() const { return paintFrame; }

	// VideoLayer
	void takeRawScreenShot(std::optional<unsigned> height, const std::string& filename) override;

	[[nodiscard]] CliComm& getCliComm();

private:
	[[nodiscard]] static unsigned getLineWidth(FrameSource* frame, unsigned y, unsigned step);

private:
	Display& display;
	RenderSettings& renderSettings;

	/** The last 4 fully rendered (unscaled) MSX frames. */
	std::array<std::unique_ptr<RawFrame>, 4> lastFrames;
	std::unique_ptr<DeinterlacedFrame> deinterlacedFrame;
	std::unique_ptr<DoubledFrame> interlacedFrame;
	std::unique_ptr<Deflicker> deflicker;
	std::unique_ptr<SuperImposedFrame> superImposedFrame;

	/** The frame as it should be displayed. */
	FrameSource* paintFrame = nullptr;

	AviRecorder* recorder = nullptr;

	const RawFrame* superImposeVideoFrame = nullptr;
	const FrameSource* superImposeVdpFrame = nullptr;

	int lastFramesCount = 0; // How many items in lastFrames[] are up-to-date
	unsigned maxWidth; // we lazily create RawFrame objects in lastFrames[]
	unsigned height;   // these two vars remember how big those should be

	/** Laserdisc cannot do interlace. */
	const bool canDoInterlace;
};

} // namespace openmsx

#endif // KAIRO_POSTPROCESSOR_HH
