#include "KairoVideoSystem.hh"

#include "CommandException.hh"
#include "Display.hh"
#include "KairoHost.hh"
#include "MSXMotherBoard.hh"
#include "PostProcessor.hh"
#include "Reactor.hh"
#include "SDLRasterizer.hh"
#include "V9990.hh"
#include "V9990SDLRasterizer.hh"
#include "VDP.hh"

#include <cassert>

namespace openmsx {

KairoVideoSystem::KairoVideoSystem(Reactor& reactor)
	: display(reactor.getDisplay())
	, host(KairoHost::current())
{
	assert(!host.surface);
	host.surface = &screen;
}

KairoVideoSystem::~KairoVideoSystem()
{
	host.surface = nullptr;
}

// The rasterizers and frame sizes are SDLVideoSystem's.
std::unique_ptr<Rasterizer> KairoVideoSystem::createRasterizer(VDP& vdp)
{
	std::string videoSource = (vdp.getName() == "VDP")
	                        ? "MSX" // for backwards compatibility
	                        : vdp.getName();
	auto& motherBoard = vdp.getMotherBoard();
	return std::make_unique<SDLRasterizer>(
		vdp, display, screen,
		std::make_unique<PostProcessor>(
			motherBoard, display, screen,
			videoSource, 640, 240, true));
}

std::unique_ptr<V9990Rasterizer> KairoVideoSystem::createV9990Rasterizer(
	V9990& vdp)
{
	std::string videoSource = (vdp.getName() == "Sunrise GFX9000")
	                        ? "GFX9000" // for backwards compatibility
	                        : vdp.getName();
	MSXMotherBoard& motherBoard = vdp.getMotherBoard();
	return std::make_unique<V9990SDLRasterizer>(
		vdp, display, screen,
		std::make_unique<PostProcessor>(
			motherBoard, display, screen,
			videoSource, 1280, 240, true));
}

void KairoVideoSystem::flush()
{
	// The host takes the picture from the surface after each frame.
}

void KairoVideoSystem::takeScreenShot(const std::string& filename, bool /*withOsd*/)
{
	// There is no on-screen display to leave out.
	screen.saveScreenshot(filename);
}

std::optional<gl::ivec2> KairoVideoSystem::getMouseCoord()
{
	return std::nullopt; // no host mouse pointer
}

OutputSurface* KairoVideoSystem::getOutputSurface()
{
	return &screen;
}

void KairoVideoSystem::showCursor(bool /*show*/)
{
}

bool KairoVideoSystem::getCursorEnabled()
{
	return false;
}

std::string KairoVideoSystem::getClipboardText()
{
	throw CommandException("KairoMSX has no clipboard");
}

void KairoVideoSystem::setClipboardText(zstring_view /*text*/)
{
	throw CommandException("KairoMSX has no clipboard");
}

std::optional<gl::ivec2> KairoVideoSystem::getWindowPosition()
{
	return std::nullopt; // no window
}

void KairoVideoSystem::setWindowPosition(gl::ivec2 /*pos*/)
{
}

void KairoVideoSystem::repaint()
{
	display.repaintImpl(screen);
}

} // namespace openmsx
