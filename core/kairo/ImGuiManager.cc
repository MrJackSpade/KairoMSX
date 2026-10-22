#include "ImGuiManager.hh"

#include "ImGuiWatchExpr.hh"

namespace openmsx {

ImGuiManager::ImGuiManager(Reactor& /*reactor*/)
	: watchExpr(std::make_unique<ImGuiWatchExpr>())
{
}

ImGuiManager::~ImGuiManager() = default;

} // namespace openmsx
