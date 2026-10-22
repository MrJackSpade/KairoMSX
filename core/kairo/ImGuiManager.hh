// openMSX's ImGuiManager for KairoMSX, which has no ImGui windows: KairoMSX's frontend is the
// user interface. It keeps what the emulation core asks of the manager, the window position
// setting and the debugger's watch expressions, and draws nothing. KairoMSX's build compiles
// none of openMSX's imgui/ windows, so this header stands in for imgui/ImGuiManager.hh.

#ifndef KAIRO_IMGUI_MANAGER_HH
#define KAIRO_IMGUI_MANAGER_HH

// Code that includes openMSX's header gets Shortcuts through it.
#include "Shortcuts.hh"

#include "gl_vec.hh"

#include <memory>

namespace openmsx {

class ImGuiWatchExpr;
class Reactor;

class ImGuiManager
{
public:
	explicit ImGuiManager(Reactor& reactor);
	~ImGuiManager();

	void storeWindowPosition(gl::ivec2 pos) { windowPos = pos; }
	[[nodiscard]] gl::ivec2 retrieveWindowPosition() const { return windowPos; }

	std::unique_ptr<ImGuiWatchExpr> watchExpr;

private:
	gl::ivec2 windowPos;
};

} // namespace openmsx

#endif // KAIRO_IMGUI_MANAGER_HH
