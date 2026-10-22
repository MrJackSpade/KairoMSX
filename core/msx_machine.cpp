#include "msx_machine.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <string_view>

#include "CommandController.hh"
#include "CommandLineParser.hh"
#include "Display.hh"
#include "Event.hh"
#include "EventDistributor.hh"
#include "EventListener.hh"
#include "KairoHost.hh"
#include "KairoOutputSurface.hh"
#include "MSXEventDistributor.hh"
#include "MSXException.hh"
#include "MSXMixer.hh"
#include "MSXMotherBoard.hh"
#include "Reactor.hh"
#include "RenderSettings.hh"
#include "TclObject.hh"
#include "Thread.hh"

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
};

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

struct MsxMachine::Impl {
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
};

MsxMachine::MsxMachine(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

MsxMachine::~MsxMachine() = default;

std::unique_ptr<MsxMachine> MsxMachine::create(const Config &config, std::string &error) {
    // openMSX reads its folders from these once per process.
    set_environment("OPENMSX_SYSTEM_DATA", config.data_dir);
    set_environment("OPENMSX_HOME", config.home);
    Thread::setMainThread();

    auto impl = std::make_unique<Impl>();
    impl->host.sampleRate = unsigned(config.sample_rate);
    try {
        impl->reactor = std::make_unique<Reactor>();
        Reactor &reactor = *impl->reactor;

        // The machine starts as openMSX's own command line would start it.
        std::vector<std::string> args = {"openmsx", "-machine", config.machine};
        for (const auto &extension : config.extensions) {
            args.push_back("-ext");
            args.push_back(extension);
        }
        for (int slot = 0; slot < kSlots; ++slot) {
            if (config.media[slot].empty()) continue;
            args.push_back(kSlotCommands[slot].option);
            args.push_back(config.media[slot]);
        }
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

std::string MsxMachine::media_path(Slot slot) {
    // openMSX's own report of a slot's medium: its "target" is the image or extension, absent or
    // empty for an empty slot.
    TclObject info = impl_->execute(tcl_command({"machine_info", "media", kSlotCommands[slot].media}));
    auto target = info.getOptionalDictValue(TclObject("target"));
    return target ? std::string(target->getString()) : std::string();
}

} // namespace kairomsx
