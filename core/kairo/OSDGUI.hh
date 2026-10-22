// openMSX's on-screen display (console/OSDGUI.hh) for KairoMSX, which draws no OSD: there is no
// "osd" command, so a script that draws OSD widgets fails with an unknown command. openMSX only
// loads such scripts on demand (share/scripts/_*.tcl); KairoMSX's build compiles none of the
// console/OSD* widgets, so this header stands in for console/OSDGUI.hh.

#ifndef KAIRO_OSDGUI_HH
#define KAIRO_OSDGUI_HH

namespace openmsx {

class CommandController;
class Display;

class OSDGUI
{
public:
	OSDGUI(CommandController& /*commandController*/, Display& display_) : display(display_) {}

	[[nodiscard]] Display& getDisplay() const { return display; }

private:
	Display& display;
};

} // namespace openmsx

#endif // KAIRO_OSDGUI_HH
