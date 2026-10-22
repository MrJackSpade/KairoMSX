// openMSX's RendererFactory (video/RendererFactory.hh) for KairoMSX: the default renderer
// (SDLGL-PP) renders through KairoVideoSystem.

#include "RendererFactory.hh"

#include "Display.hh"
#include "DummyRenderer.hh"
#include "DummyVideoSystem.hh"
#include "KairoVideoSystem.hh"
#include "PixelRenderer.hh"
#include "Reactor.hh"
#include "RenderSettings.hh"
#include "V9990DummyRenderer.hh"
#include "V9990PixelRenderer.hh"

#include "unreachable.hh"

namespace openmsx::RendererFactory {

std::unique_ptr<VideoSystem> createVideoSystem(Reactor& reactor)
{
	Display& display = reactor.getDisplay();
	switch (display.getRenderer()) {
		case RenderSettings::RendererID::DUMMY:
			return std::make_unique<DummyVideoSystem>();
		case RenderSettings::RendererID::SDLGL_PP:
			return std::make_unique<KairoVideoSystem>(reactor);
		default:
			UNREACHABLE;
	}
}

std::unique_ptr<Renderer> createRenderer(VDP& vdp, Display& display)
{
	switch (display.getRenderer()) {
		case RenderSettings::RendererID::DUMMY:
			return std::make_unique<DummyRenderer>();
		case RenderSettings::RendererID::SDLGL_PP:
			return std::make_unique<PixelRenderer>(vdp, display);
		default:
			UNREACHABLE;
	}
}

std::unique_ptr<V9990Renderer> createV9990Renderer(V9990& vdp, Display& display)
{
	switch (display.getRenderer()) {
		case RenderSettings::RendererID::DUMMY:
			return std::make_unique<V9990DummyRenderer>();
		case RenderSettings::RendererID::SDLGL_PP:
			return std::make_unique<V9990PixelRenderer>(vdp);
		default:
			UNREACHABLE;
	}
}

} // namespace openmsx::RendererFactory
