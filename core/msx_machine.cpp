#include "msx_machine.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string_view>

#include "CommandController.hh"
#include "CommandLineParser.hh"
#include "Connector.hh"
#include "Display.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "EventListener.hh"
#include "Interpreter.hh"
#include "KairoHost.hh"
#include "KairoOutputSurface.hh"
#include "MSXEventDistributor.hh"
#include "MSXException.hh"
#include "MSXMixer.hh"
#include "GlobalSettings.hh"
#include "Keyboard.hh"
#include "Mouse.hh"
#include "kairo/relative_warp.h"
#include "LedStatus.hh"
#include "MSXMotherBoard.hh"
#include "Pluggable.hh"
#include "PluggingController.hh"
#include "Reactor.hh"
#include "ThrottleManager.hh"
#include "RenderSettings.hh"
#include "TclObject.hh"
#include "Thread.hh"

// Toggles of a joystick port's pin 8 (patch group openmsx-mouse-strobes).
namespace openmsx { extern unsigned long kairoMouseStrobes; }

namespace kairomsx {

using namespace openmsx;

namespace {

// openMSX's console commands for each Slot: the command line option that loads it at start, and
// the command that changes it.
struct SlotCommands {
    const char *option;
    const char *insert;
    const char *eject;
    // The slot's name for "machine_info media".
    const char *media;
};
constexpr SlotCommands kSlotCommands[kSlots] = {
    {"-carta", "carta insert", "carta eject", "carta"},
    {"-cartb", "cartb insert", "cartb eject", "cartb"},
    {"-diska", "diska insert", "diska eject", "diska"},
    {"-diskb", "diskb insert", "diskb eject", "diskb"},
    {"-cassetteplayer", "cassetteplayer insert", "cassetteplayer eject", "cassetteplayer"},
    // openMSX changes a hard disk only with the machine off (insert) and has none to take out.
    {"-hda", "hda insert", nullptr, "hda"},
};

// The IDE interface a hard disk goes on (openMSX's extensions/SunriseIDE_Nextor.xml).
constexpr const char *kHardDiskExtension = "SunriseIDE_Nextor";

// The joystick the host's pad stands for: openMSX's joystick 1 ("joy1"), with no host joystick
// behind it. Its buttons 0 to 5 are the MSX joystick's controls, in MsxMachine::joystick's order.
constexpr int kJoystick = 0;
constexpr const char *kJoystickConfig =
    "set msxjoystick1_config {UP {{joy1 button0}} DOWN {{joy1 button1}} LEFT {{joy1 button2}} "
    "RIGHT {{joy1 button3}} A {{joy1 button4}} B {{joy1 button5}}}";

// A Tcl command from its words, quoted as Tcl needs.
std::string tcl_command(std::initializer_list<std::string_view> words) {
    TclObject command;
    for (auto word : words) command.addListElement(word);
    return std::string(command.getString());
}

void set_environment(const char *name, const std::string &value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

// The machine to start for config: config.machine, or for a memory size its variant with that much
// main RAM, "<machine>_<size>KB". The variants of every size are written beside the machine's own
// file, where its ROM files resolve, so a saved state of any size finds its machine. False with error
// set when the machine's file cannot be read or a variant written.
bool memory_machine(const Config &config, std::string &machine, std::string &error) {
    machine = config.machine;
    if (config.memory_kb == 0) return true;
    if (std::find(std::begin(kMemorySizes), std::end(kMemorySizes), config.memory_kb) == std::end(kMemorySizes)) {
        error = "unsupported memory size " + std::to_string(config.memory_kb) + " KB";
        return false;
    }
    const std::filesystem::path machines = std::filesystem::path(config.data_dir) / "machines";
    std::ifstream in(machines / (config.machine + ".xml"), std::ios::binary);
    if (!in) {
        error = "cannot read the machine " + config.machine;
        return false;
    }
    const std::string xml((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // The main RAM's size: <size> inside <MemoryMapper id="Main RAM">.
    const size_t mapper = xml.find("<MemoryMapper id=\"Main RAM\">");
    if (mapper == std::string::npos) return true;
    const size_t end = xml.find("</MemoryMapper>", mapper);
    const size_t size = xml.find("<size>", mapper);
    const size_t size_end = xml.find("</size>", mapper);
    if (end == std::string::npos || size == std::string::npos || size_end == std::string::npos || size_end > end) {
        error = "the machine " + config.machine + " has no main RAM size";
        return false;
    }
    for (int kb : kMemorySizes) {
        std::string variant = xml;
        variant.replace(size + 6, size_end - size - 6, std::to_string(kb));
        const std::filesystem::path file = machines / (config.machine + "_" + std::to_string(kb) + "KB.xml");
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!(out << variant)) {
            error = "cannot write " + file.string();
            return false;
        }
    }
    machine = config.machine + "_" + std::to_string(config.memory_kb) + "KB";
    return true;
}

// Counts the frames the shown VDP finishes, drawn or not: openMSX does not draw the first frame
// after a machine starts or a state loads.
class FrameCounter final : public EventListener {
public:
    explicit FrameCounter(EventDistributor &distributor) : distributor_(distributor) {
        distributor_.registerEventListener(EventType::FINISH_FRAME, *this);
    }
    ~FrameCounter() { distributor_.unregisterEventListener(EventType::FINISH_FRAME, *this); }

    uint64_t frames() const { return frames_; }

private:
    bool signalEvent(const Event &event) override {
        const auto &finished = get_event<FinishFrameEvent>(event);
        if (finished.getSource() == finished.getSelectedSource()) ++frames_;
        return false;
    }

    EventDistributor &distributor_;
    uint64_t frames_ = 0;
};

} // namespace

// openMSX's main thread is the one a machine is created on, until the machine (and with it openMSX)
// is gone; then another thread may run the next machine.
struct MainThreadRole {
    MainThreadRole() { Thread::setMainThread(); }
    ~MainThreadRole() { Thread::clearMainThread(); }
    MainThreadRole(const MainThreadRole &) = delete;
    MainThreadRole &operator=(const MainThreadRole &) = delete;
};

struct MsxMachine::Impl {
    // First, so it is given up last.
    MainThreadRole main_thread;
    KairoHost host;
    // openMSX's video and sound find the host through this while openMSX lives.
    KairoHost::Install install{host};
    std::unique_ptr<Reactor> reactor;
    std::unique_ptr<FrameCounter> frames;
    EmuTime frame_start = EmuTime::zero();
    double frame_seconds = 1.0 / 60.0;

    MSXMotherBoard &board() {
        MSXMotherBoard *board = reactor->getMotherBoard();
        if (!board) throw MSXException("no machine");
        return *board;
    }

    TclObject execute(const std::string &command) {
        return reactor->getCommandController().executeCommand(command);
    }

    // The MSX mouse goes into port B when the host first uses it, not at the start: a game that
    // reads port B as a joystick takes an idle mouse for all four directions held, and some, such
    // as Dragon Quest 1, wait for them to be let go (KairoMSX#6). It is looked up each time
    // because a loaded state brings its own port B.
    bool mouse_plugged() {
        Connector *port = board().getPluggingController().findConnector("joyportb");
        if (!port) throw MSXException("the machine has no joystick port B");
        return port->getPlugged().getName() == "mouse";
    }
    void plug_mouse() {
        if (!mouse_plugged()) execute("plug joyportb mouse");
    }

    // Direct touch (MsxMachine::mouse_warp).
    kairo::RelativeWarp warp;
    // Whether the MSX has read all the motion the mouse in port B was given.
    bool mouse_idle() {
        Connector *port = board().getPluggingController().findConnector("joyportb");
        if (!port) throw MSXException("the machine has no joystick port B");
        const auto *mouse = dynamic_cast<const Mouse *>(&port->getPlugged());
        if (!mouse) throw MSXException("the mouse is not in joystick port B");
        return mouse->kairoIdle();
    }
    // Motion in MSX counts: openMSX takes two host pixels a count (Mouse.cc SCALE).
    void move_counts(int dx, int dy) {
        SDL_Event event{};
        event.motion.type = SDL_MOUSEMOTION;
        event.motion.xrel = dx * 2;
        event.motion.yrel = dy * 2;
        plug_mouse();
        MSXMotherBoard &motherboard = board();
        motherboard.getMSXEventDistributor().distributeEvent(MouseMotionEvent(event), motherboard.getCurrentTime());
    }
};

MsxMachine::MsxMachine(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

MsxMachine::~MsxMachine() = default;

std::unique_ptr<MsxMachine> MsxMachine::create(const Config &config, std::string &error) {
    // openMSX reads its folders from these once per process. Its temporary files (uncompressed
    // copies of compressed images) go to TMPDIR, which Android does not set.
    set_environment("OPENMSX_SYSTEM_DATA", config.data_dir);
    set_environment("OPENMSX_HOME", config.home);
    const std::string temp = config.home + "/tmp";
    std::filesystem::create_directories(temp);
    set_environment("TMPDIR", temp);

    auto impl = std::make_unique<Impl>();
    impl->host.sampleRate = unsigned(config.sample_rate);
    try {
        impl->reactor = std::make_unique<Reactor>();
        Reactor &reactor = *impl->reactor;

        // The machine starts as openMSX's own command line would start it.
        std::string machine;
        if (!memory_machine(config, machine, error)) return nullptr;
        std::vector<std::string> args = {"openmsx", "-machine", machine};
        for (const auto &extension : config.extensions) {
            args.push_back("-ext");
            args.push_back(extension);
        }
        if (!config.media[kHardDisk].empty()) {
            args.push_back("-ext");
            args.push_back(kHardDiskExtension);
        }
        for (int slot = 0; slot < kSlots; ++slot) {
            if (config.media[slot].empty()) continue;
            args.push_back(kSlotCommands[slot].option);
            args.push_back(config.media[slot]);
        }
        // openMSX runs the Z80 at the machine's own clock until its frequency is unlocked.
        if (config.z80_hz) {
            args.push_back("-command");
            args.push_back("set z80_freq_locked false; set z80_freq " + std::to_string(config.z80_hz));
        }
        // A loading drive or tape unthrottles openMSX, which loading() reports to the host.
        args.push_back("-command");
        args.push_back("set fullspeedwhenloading on");
        // The host shows no reverse bar, so openMSX keeps no reverse history.
        args.push_back("-command");
        args.push_back("set auto_enable_reverse off");
        // The host's pad is an MSX joystick in port A, its controls the buttons of joystick 1
        // (see joystick()).
        args.push_back("-command");
        args.push_back(kJoystickConfig);
        args.push_back("-command");
        args.push_back("plug joyporta msxjoystick1");
        std::vector<char *> argv;
        for (auto &arg : args) argv.push_back(arg.data());

        // parse() also initializes the reactor.
        CommandLineParser parser(reactor);
        parser.parse(argv);
        if (parser.getParseStatus() != CommandLineParser::Status::RUN) {
            error = "openMSX did not accept its start options";
            return nullptr;
        }
        reactor.runStartupScripts(parser);
        auto &renderer = reactor.getDisplay().getRenderSettings().getRendererSetting();
        if (renderer.getEnum() == RenderSettings::RendererID::UNINITIALIZED) {
            renderer.setValue(renderer.getDefaultValue());
            reactor.getEventDistributor().deliverEvents();
        }
        impl->frames = std::make_unique<FrameCounter>(reactor.getEventDistributor());
        reactor.powerOn();
        impl->frame_start = impl->board().getCurrentTime();
    } catch (MSXException &e) {
        error = e.getMessage();
        return nullptr;
    } catch (FatalError &e) {
        // openMSX's start reports what it could not do, such as a start command that failed.
        error = e.getMessage();
        return nullptr;
    }
    return std::unique_ptr<MsxMachine>(new MsxMachine(std::move(impl)));
}

bool MsxMachine::run_frame(std::string &error) {
    try {
        Reactor &reactor = *impl_->reactor;
        KairoHost &host = impl_->host;
        if (!host.surface) throw MSXException("openMSX has no video system");
        // A frame ends when the shown VDP finishes it; delivering its event paints it.
        if (impl_->warp.running()) {
            int dx, dy;
            impl_->warp.step(impl_->mouse_idle(), dx, dy);
            if (dx || dy) impl_->move_counts(dx, dy);
        }
        uint64_t frames = impl_->frames->frames();
        while (true) {
            reactor.getEventDistributor().deliverEvents();
            if (impl_->frames->frames() != frames) break;
            // MSXMotherBoard::execute runs until an event is due, such as the VDP's end of frame.
            if (!impl_->board().execute()) throw MSXException("the machine is off");
        }
        EmuTime now = impl_->board().getCurrentTime();
        // openMSX mixes in fragments of the "samples" setting; the host takes each frame's sound
        // with the frame.
        impl_->board().getMSXMixer().updateStream(now);
        impl_->frame_seconds = (now - impl_->frame_start).toDouble();
        impl_->frame_start = now;
        return true;
    } catch (MSXException &e) {
        error = e.getMessage();
        return false;
    }
}

double MsxMachine::frame_seconds() const { return impl_->frame_seconds; }

const uint8_t *MsxMachine::frame(uint32_t &width, uint32_t &height) const {
    const KairoOutputSurface *surface = impl_->host.surface;
    width = surface->getWidth();
    height = surface->getHeight();
    return surface->getPixels();
}

int MsxMachine::sample_rate() const { return int(impl_->host.sampleRate); }

uint32_t MsxMachine::read_audio(int16_t *stereo, uint32_t max_frames) {
    auto &audio = impl_->host.audio;
    uint32_t frames = std::min<uint32_t>(max_frames, uint32_t(audio.size() / 2));
    std::copy_n(audio.begin(), frames * 2, stereo);
    audio.erase(audio.begin(), audio.begin() + frames * 2);
    return frames;
}

void MsxMachine::key(int row, int column, bool down) {
    impl_->execute(std::string(down ? "keymatrixdown " : "keymatrixup ") + std::to_string(row) + " " +
                   std::to_string(1 << column));
}

bool MsxMachine::typed_key(char c, int &code, bool &shift) {
    const Keyboard *keyboard = impl_->board().getKeyboard();
    if (!keyboard) throw MSXException("the machine has no keyboard");
    const auto info = keyboard->getUnicodeKeymap().get(static_cast<unsigned char>(c));
    if (!info.isValid()) return false;
    // A character needing Ctrl, Graph or Code is not typed; Caps only says the key follows it.
    using Info = UnicodeKeymap::KeyInfo;
    if (info.modMask & (Info::CTRL_MASK | Info::GRAPH_MASK | Info::CODE_MASK)) return false;
    code = info.pos.getRow() * 8 + info.pos.getColumn() + 1;
    shift = (info.modMask & Info::SHIFT_MASK) != 0;
    return true;
}

std::string MsxMachine::keyboard_type() {
    const Keyboard *keyboard = impl_->board().getKeyboard();
    if (!keyboard) throw MSXException("the machine has no keyboard");
    return keyboard->getKairoKeyboardType();
}

int MsxMachine::special_key(kairo::SpecialKey key) {
    switch (key) {
    case kairo::SpecialKey::Return: return 7 * 8 + 7 + 1;
    case kairo::SpecialKey::Shift: return 6 * 8 + 0 + 1;
    case kairo::SpecialKey::Ctrl: return 6 * 8 + 1 + 1;
    case kairo::SpecialKey::F1: return 6 * 8 + 5 + 1;
    case kairo::SpecialKey::F2: return 6 * 8 + 6 + 1;
    case kairo::SpecialKey::F3: return 6 * 8 + 7 + 1;
    case kairo::SpecialKey::F4: return 7 * 8 + 0 + 1;
    case kairo::SpecialKey::F5: return 7 * 8 + 1 + 1;
    }
    return -1;
}

// The prompt is the last line of text with something on it: MSX-BASIC's Ok, or MSX-DOS's drive
// prompt (A>), with the cursor (a character outside ASCII) after it. Only the text modes have lines.
// While BASIC shows the function keys (CNSDFG, F3DEh, not 0) they take the screen's last row, which
// is then not a line of text.
kairo::Prompt MsxMachine::prompt() {
    const std::string mode(impl_->execute("get_screen_mode").getString());
    if (mode != "TEXT40" && mode != "TEXT80" && mode != "1") return kairo::Prompt::None;
    const std::string screen(impl_->execute("get_screen").getString());
    std::vector<std::string> lines;
    std::string::size_type begin = 0;
    while (begin < screen.size()) {
        auto end = screen.find('\n', begin);
        if (end == std::string::npos) end = screen.size();
        std::string line;
        for (auto i = begin; i < end; ++i)
            if (screen[i] > ' ' && screen[i] <= '~') line += screen[i];
        lines.push_back(line);
        begin = end + 1;
    }
    if (!lines.empty() && std::string(impl_->execute("peek 0xF3DE").getString()) != "0") lines.pop_back();
    std::string last;
    for (const std::string &line : lines)
        if (!line.empty()) last = line;
    const bool drive = last.size() == 2 && last[0] >= 'A' && last[0] <= 'H' && last[1] == '>';
    return last == "Ok" || drive ? kairo::Prompt::Line : kairo::Prompt::None;
}

void MsxMachine::joystick(int control, bool down) {
    // The joystick's control as a button of joystick 1, which kJoystickConfig binds; openMSX
    // passes it on, at the current time, as it passes on a host joystick's events.
    SDL_Event event{};
    event.jbutton.type = down ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
    event.jbutton.which = kJoystick;
    event.jbutton.button = uint8_t(control);
    event.jbutton.state = down ? SDL_PRESSED : SDL_RELEASED;
    MSXMotherBoard &board = impl_->board();
    if (down) {
        board.getMSXEventDistributor().distributeEvent(JoystickButtonDownEvent(event), board.getCurrentTime());
    } else {
        board.getMSXEventDistributor().distributeEvent(JoystickButtonUpEvent(event), board.getCurrentTime());
    }
}

bool MsxMachine::command(const std::string &command, std::string &result) {
    try {
        result = std::string(impl_->execute(command).getString());
        return true;
    } catch (MSXException &e) {
        result = e.getMessage();
        return false;
    }
}

void MsxMachine::reset() { impl_->execute("reset"); }

// The throttle stays on (KairoMSX's RealTime paces the frames), so openMSX unthrottles only while a
// device loads (ThrottleManager::isThrottled).
unsigned long MsxMachine::mouse_strobes() const { return openmsx::kairoMouseStrobes; }

bool MsxMachine::loading() { return !impl_->reactor->getGlobalSettings().getThrottleManager().isThrottled(); }

int MsxMachine::locks() {
    const LedStatus &leds = impl_->board().getLedStatus();
    return (leds.getLed(LedStatus::CAPS) ? 1 : 0) | (leds.getLed(LedStatus::KANA) ? 2 : 0);
}

bool MsxMachine::save_state(const std::string &file, std::string &error) {
    try {
        impl_->execute(tcl_command({"store_machine", impl_->board().getMachineID(), file}));
        return true;
    } catch (MSXException &e) {
        error = e.getMessage();
        return false;
    }
}

// As openMSX's loadstate (share/scripts/_savestate.tcl): the saved machine replaces the running one.
bool MsxMachine::load_state(const std::string &file, std::string &error) {
    try {
        std::string loaded(impl_->execute(tcl_command({"restore_machine", file})).getString());
        impl_->execute(tcl_command({"delete_machine", impl_->board().getMachineID()}));
        impl_->execute(tcl_command({"activate_machine", loaded}));
        impl_->frame_start = impl_->board().getCurrentTime();
        return true;
    } catch (MSXException &e) {
        error = e.getMessage();
        return false;
    }
}

bool MsxMachine::insert(Slot slot, const std::string &path, std::string &error) {
    if (slot == kHardDisk) return insert_hard_disk(path, error);
    try {
        if (path.empty()) {
            impl_->execute(kSlotCommands[slot].eject);
        } else {
            impl_->execute(std::string(kSlotCommands[slot].insert) + " " + tcl_command({path}));
        }
        return true;
    } catch (MSXException &e) {
        error = e.getMessage();
        return false;
    }
}

// The picture is the screen inside openMSX's borders: 256 pixels across in 320 (twice that in a
// 512-pixel mode), and 212 or 192 lines (VDP register 9's LN) in 240, both centred.
void MsxMachine::mouse_warp(int x, int y) {
    uint32_t width = 0, height = 0;
    if (!frame(width, height)) throw MSXException("no picture to point at yet");
    const int across = int(width) / 320, down = int(height) / 240;
    const int lines = (std::stoi(std::string(impl_->execute("vdpreg 9").getString())) & 0x80) ? 212 : 192;
    impl_->plug_mouse();
    impl_->warp.start(x / across - 32, y / down - (240 - lines) / 2, 256, lines);
}

bool MsxMachine::mouse_warp_done() const { return impl_->warp.done(); }

// The mouse's events go to the machine at the current time, as openMSX passes on a host mouse's.
void MsxMachine::mouse_move(int dx, int dy) {
    // A drag after a direct tap goes with the warp's counts until they are given.
    if (impl_->warp.hold(dx / 2, dy / 2)) return;
    SDL_Event event{};
    event.motion.type = SDL_MOUSEMOTION;
    event.motion.xrel = dx;
    event.motion.yrel = dy;
    impl_->plug_mouse();
    MSXMotherBoard &board = impl_->board();
    board.getMSXEventDistributor().distributeEvent(MouseMotionEvent(event), board.getCurrentTime());
}

void MsxMachine::mouse_button(int button, bool down) {
    SDL_Event event{};
    event.button.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.button = button == 2 ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
    event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    // Letting go of a button (as release_inputs does) plugs nothing in.
    if (down) impl_->plug_mouse();
    else if (!impl_->mouse_plugged()) return;
    MSXMotherBoard &board = impl_->board();
    if (down) {
        board.getMSXEventDistributor().distributeEvent(MouseButtonDownEvent(event), board.getCurrentTime());
    } else {
        board.getMSXEventDistributor().distributeEvent(MouseButtonUpEvent(event), board.getCurrentTime());
    }
}

// The machine restarts with the new hard disk: openMSX changes one only with the power off.
bool MsxMachine::insert_hard_disk(const std::string &path, std::string &error) {
    if (path.empty()) {
        error = "A hard disk cannot be taken out of a running MSX; start another game instead";
        return false;
    }
    if (!has_hard_disk()) {
        error = "This MSX started without a hard disk interface; start the hard disk as a game instead";
        return false;
    }
    try {
        impl_->execute("set power off");
        impl_->execute(std::string(kSlotCommands[kHardDisk].insert) + " " + tcl_command({path}));
        impl_->execute("set power on");
        return true;
    } catch (MSXException &e) {
        error = e.getMessage();
        return false;
    }
}

// Whether the machine has the IDE interface, whose hda command reports its image.
bool MsxMachine::has_hard_disk() {
    return std::string(impl_->execute(tcl_command({"info", "commands", "hda"})).getString()) == "hda";
}

std::string MsxMachine::media_path(Slot slot) {
    if (slot == kHardDisk) {
        if (!has_hard_disk()) return {};
        // "hda: <image>", and the options of a read-only one.
        TclObject result = impl_->execute(kSlotCommands[kHardDisk].media);
        return std::string(result.getListIndex(impl_->reactor->getInterpreter(), 1).getString());
    }
    // A slot the machine does not have, such as a disk drive on C-BIOS, holds nothing.
    const std::string_view name = kSlotCommands[slot].media;
    TclObject slots = impl_->execute(tcl_command({"machine_info", "media"}));
    Interpreter &interpreter = impl_->reactor->getInterpreter();
    bool present = false;
    for (size_t i = 0; i < slots.getListLength(interpreter); ++i)
        present = present || slots.getListIndex(interpreter, i).getString() == name;
    if (!present) return {};
    // openMSX's own report of a slot's medium: its "target" is the image or extension, absent or
    // empty for an empty slot.
    TclObject info = impl_->execute(tcl_command({"machine_info", "media", name}));
    auto target = info.getOptionalDictValue(TclObject("target"));
    return target ? std::string(target->getString()) : std::string();
}

} // namespace kairomsx
