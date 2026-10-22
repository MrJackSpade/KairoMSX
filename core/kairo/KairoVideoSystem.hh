// openMSX's video system for KairoMSX: renders each VDP with openMSX's software rasterizers into
// a KairoOutputSurface the host reads, in place of SDLVideoSystem's window. RendererFactory
// creates it for the default (SDLGL-PP) renderer.

#ifndef KAIRO_VIDEOSYSTEM_HH
#define KAIRO_VIDEOSYSTEM_HH

#include "KairoOutputSurface.hh"
#include "VideoSystem.hh"

namespace openmsx {

class Display;
struct KairoHost;
class Reactor;

class KairoVideoSystem final : public VideoSystem
{
public:
	explicit KairoVideoSystem(Reactor& reactor);
	~KairoVideoSystem() override;

	// VideoSystem interface:
	[[nodiscard]] std::unique_ptr<Rasterizer> createRasterizer(VDP& vdp) override;
	[[nodiscard]] std::unique_ptr<V9990Rasterizer> createV9990Rasterizer(
		V9990& vdp) override;
	void flush() override;
	void takeScreenShot(const std::string& filename, bool withOsd) override;
	[[nodiscard]] std::optional<gl::ivec2> getMouseCoord() override;
	[[nodiscard]] OutputSurface* getOutputSurface() override;
	void showCursor(bool show) override;
	[[nodiscard]] bool getCursorEnabled() override;
	[[nodiscard]] std::string getClipboardText() override;
	void setClipboardText(zstring_view text) override;
	[[nodiscard]] std::optional<gl::ivec2> getWindowPosition() override;
	void setWindowPosition(gl::ivec2 pos) override;
	void repaint() override;

private:
	Display& display;
	KairoHost& host;
	KairoOutputSurface screen;
};

} // namespace openmsx

#endif // KAIRO_VIDEOSYSTEM_HH
