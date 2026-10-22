// The key chord names of openMSX's ImGui shortcuts (imgui/ImGuiUtils.cc), for the settings file.
// The rest of ImGuiUtils.cc draws ImGui windows, which KairoMSX's build leaves out. KairoMSX
// runs no ImGui context, so the Super key is always "Super+" (openMSX names it "Cmd+" when ImGui
// is set up for macOS).

#include "ImGuiUtils.hh"
#include "KeyMappings.hh"

#include "strCat.hh"

#include <SDL.h>

using namespace std::literals;

namespace openmsx {

static constexpr std::string_view superName = "Super+"sv;

std::string getKeyChordName(ImGuiKeyChord keyChord)
{
	int keyCode = ImGuiKey2SDL(ImGuiKey(keyChord & ~ImGuiMod_Mask_));
	const auto* name = SDL_GetKeyName(keyCode);
	if (!name || (*name == '\0')) return "None";
	return strCat(
		(keyChord & ImGuiMod_Ctrl  ? "Ctrl+"sv  : ""sv),
		(keyChord & ImGuiMod_Shift ? "Shift+"sv : ""sv),
		(keyChord & ImGuiMod_Alt   ? "Alt+"sv   : ""sv),
		(keyChord & ImGuiMod_Super ? superName : ""sv),
		name);
}

std::optional<ImGuiKeyChord> parseKeyChord(std::string_view name)
{
	if (name == "None") return ImGuiKey_None;

	// Similar to "StringOp::splitOnLast(name, '+')", but includes the last '+'
	auto [modifiers, key] = [&] -> std::pair<std::string_view, std::string_view> {
		if (auto pos = name.find_last_of('+'); pos == std::string_view::npos) {
			return {std::string_view{}, name};
		} else {
			return {name.substr(0, pos + 1), name.substr(pos + 1)};
		}
	}();

	SDL_Keycode keyCode = SDL_GetKeyFromName(std::string(key).c_str());
	if (keyCode == SDLK_UNKNOWN) return {};

	ImGuiKeyChord keyMods =
		(modifiers.contains("Ctrl+" ) ? ImGuiMod_Ctrl  : 0) |
		(modifiers.contains("Shift+") ? ImGuiMod_Shift : 0) |
		(modifiers.contains("Alt+"  ) ? ImGuiMod_Alt   : 0) |
		(modifiers.contains(superName) ? ImGuiMod_Super : 0);

	return SDLKey2ImGui(keyCode) | keyMods;
}

} // namespace openmsx
