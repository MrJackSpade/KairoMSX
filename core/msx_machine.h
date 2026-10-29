// An MSX on openMSX (third_party/openmsx), run one video frame at a time by its host instead of
// openMSX's own real-time loop. openMSX draws and plays through KairoMSX's replacements
// (core/kairo/), and the host takes each frame's picture and sound from the machine.
//
// openMSX keeps process-wide state and runs on the thread that started it: a process runs one
// machine at a time, and a machine is created, used and destroyed on one thread.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "kairo/startup_typer.h"

namespace kairomsx {

// Where media go: the cartridge slots, the disk drives and the cassette player.
enum Slot { kCartA, kCartB, kDiskA, kDiskB, kCassette, kSlots };

// The joystick's controls: up, down, left, right, trigger A, trigger B.
constexpr int kJoystickControls = 6;

// The main RAM sizes, in KB, a machine with a memory mapper can start with (Config::memory_kb).
constexpr int kMemorySizes[] = {64, 128, 256, 512, 1024, 2048, 4096};

// What a machine starts with.
struct Config {
    // openMSX's data folder (share/ with the C-BIOS machines; tools/stage_openmsx_data.py).
    std::string data_dir;
    // A private folder for openMSX's own files: settings, system ROMs, temporary files.
    std::string home;
    // The machine, by openMSX's name for it (a file in <data_dir>/machines).
    std::string machine = "C-BIOS_MSX2+";
    // The main memory mapper's size in KB, one of kMemorySizes, or 0 for the machine's own. A
    // machine whose main RAM is not a plain memory mapper (MSX1 RAM, the turbo R's) keeps its own.
    int memory_kb = 0;
    // The Z80's clock in Hz (openMSX's z80_freq), or 0 for the machine's own.
    int z80_hz = 0;
    // Extensions to plug in, by openMSX's names for them (files in <data_dir>/extensions).
    std::vector<std::string> extensions;
    // The image in each Slot; "" for an empty one.
    std::string media[kSlots];
    // The audio sample rate.
    int sample_rate = 48000;
};

class MsxMachine {
public:
    // A machine running config's media; null with error set when it cannot start.
    static std::unique_ptr<MsxMachine> create(const Config &config, std::string &error);
    ~MsxMachine();

    // Runs one guest video frame; false with error set when the machine stops.
    bool run_frame(std::string &error);
    // The guest time the last frame lasted.
    double frame_seconds() const;
    // The latest picture: RGBA rows of width pixels.
    const uint8_t *frame(uint32_t &width, uint32_t &height) const;
    int sample_rate() const;
    // Takes up to max_frames interleaved stereo frames produced so far.
    uint32_t read_audio(int16_t *stereo, uint32_t max_frames);

    // A key by its place in the MSX keyboard matrix: row 0 to 10, column (bit) 0 to 7.
    void key(int row, int column, bool down);
    // A control of the joystick in port A: kJoystickControls in their order.
    void joystick(int control, bool down);
    // The mouse in port B, plugged in by its first motion or press: motion in host pixels (openMSX
    // scales it), and button 1 (left) or 2 (right).
    void mouse_move(int dx, int dy);
    void mouse_button(int button, bool down);

    // Runs an openMSX console command and returns its result; false with error set when it
    // fails.
    bool command(const std::string &command, std::string &result);

    void reset();
    // Whether a disk drive or the tape is loading (openMSX's LoadingIndicators, with its
    // fullspeedwhenloading on), which the host runs flat out.
    bool loading();
    // Startup commands (kairo::StartupTyper), with keys as the app numbers them (row * 8 + column + 1):
    // whether MSX-BASIC or MSX-DOS waits for a line, the key that types a character on this machine's
    // keyboard layout and whether Shift goes with it (false for one it has no plain key for), and the
    // special keys.
    kairo::Prompt prompt();
    bool typed_key(char c, int &code, bool &shift);
    // The machine's keyboard as its configuration names it (openMSX keyboard_type): jp_jis on a
    // Japanese MSX, int on an international one, and so on.
    std::string keyboard_type();
    static int special_key(kairo::SpecialKey key);
    // How many times the guest has toggled a joystick port's pin 8, as a mouse program does to read
    // the mouse (patch group openmsx-mouse-strobes).
    unsigned long mouse_strobes() const;
    // The lock keys on, as the machine's lamps show them: bit 0 Caps, bit 1 Kana (the Code lamp of
    // an international MSX).
    int locks();
    // Saves or loads the whole machine; false with error set when openMSX refused.
    bool save_state(const std::string &file, std::string &error);
    bool load_state(const std::string &file, std::string &error);

    // Inserts an image in a slot, or ejects it for ""; false with error set when openMSX refused.
    bool insert(Slot slot, const std::string &path, std::string &error);
    // The image in a slot, as it was inserted; "" when it is empty. An extension in a cartridge
    // slot is named by its openMSX name.
    std::string media_path(Slot slot);

private:
    struct Impl;
    explicit MsxMachine(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

} // namespace kairomsx
