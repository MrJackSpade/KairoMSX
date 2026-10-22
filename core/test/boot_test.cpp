// Boots an MSX cartridge on C-BIOS on the host and checks what the app relies on.
//
//   boot_test [--machine <name>] [--command <tcl>] <data dir> <work dir> <seconds> [cartridge [joystick|none|key:<row>,<column>]]
//
// Runs the cartridge (or C-BIOS alone) for <seconds> of guest time, writing <work dir>/boot.ppm
// of the last picture and boot.wav of the sound, then presses Space (with "joystick", the
// joystick's trigger A; with "key:<row>,<column>", that key of the keyboard matrix; with "none",
// nothing) and writes pressed.ppm. Then
// it saves a state, runs on, loads the state, and checks that every frame of the second after the
// load leaves the machine as the second after the save did, and draws and plays the same. Last it
// starts a second machine in the same process, as the app does for its next game.
// Saved states are compared as XML, without the fields state() names. With --command, it prints
// what that openMSX console command returns after the boot, as for the debugger's registers.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <zlib.h>

#include "msx_machine.h"

using kairomsx::Config;
using kairomsx::MsxMachine;

namespace {

// Space, in the MSX keyboard matrix.
constexpr int kSpaceRow = 8, kSpaceColumn = 0;
// Trigger A, in MsxMachine::joystick's order.
constexpr int kTriggerA = 4;

// A run of this many seconds runs one frame.
constexpr double kOneFrame = 1e-6;

int fail(const std::string &message) {
    std::fprintf(stderr, "FAIL: %s\n", message.c_str());
    return 1;
}

struct Run {
    std::vector<uint8_t> picture;
    uint32_t width = 0, height = 0;
    uint64_t audio_frames = 0, loud_frames = 0;
    double seconds = 0;
    std::vector<int16_t> sound;
};

bool run(MsxMachine &machine, double seconds, Run &result, std::string &error) {
    std::vector<int16_t> audio(4096 * 2);
    const double start = result.seconds;
    while (result.seconds - start < seconds) {
        if (!machine.run_frame(error)) return false;
        result.seconds += machine.frame_seconds();
        uint32_t got;
        while ((got = machine.read_audio(audio.data(), 4096)) > 0) {
            result.audio_frames += got;
            result.sound.insert(result.sound.end(), audio.begin(), audio.begin() + got * 2);
            for (uint32_t f = 0; f < got; ++f)
                if (std::abs(audio[f * 2]) > 64 || std::abs(audio[f * 2 + 1]) > 64) ++result.loud_frames;
        }
    }
    const uint8_t *pixels = machine.frame(result.width, result.height);
    result.picture.assign(pixels, pixels + result.width * result.height * 4);
    return true;
}

std::vector<char> read_file(const std::string &file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// A saved state's XML (openMSX gzips it).
std::string unzip(const std::vector<char> &gz) {
    z_stream stream{};
    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) return {};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(gz.data()));
    stream.avail_in = static_cast<uInt>(gz.size());
    std::string xml;
    char buffer[65536];
    int status;
    do {
        stream.next_out = reinterpret_cast<Bytef *>(buffer);
        stream.avail_out = sizeof(buffer);
        status = inflate(&stream, Z_NO_FLUSH);
        xml.append(buffer, sizeof(buffer) - stream.avail_out);
    } while (status == Z_OK);
    inflateEnd(&stream);
    return status == Z_STREAM_END ? xml : std::string();
}

// Removes the text from the first `from` at or after `start` through the following `to`.
void cut(std::string &xml, size_t start, const std::string &from, const std::string &to) {
    size_t begin = xml.find(from, start);
    if (begin == std::string::npos) return;
    size_t end = xml.find(to, begin + from.size());
    if (end != std::string::npos) xml.erase(begin, end + to.size() - begin);
}

// Removes every element named `name`.
void cut_all(std::string &xml, const std::string &name) {
    const std::string from = "<" + name, to = "</" + name + ">";
    for (size_t begin; (begin = xml.find(from)) != std::string::npos;) {
        size_t end = xml.find(to, begin);
        if (end == std::string::npos) return;
        xml.erase(begin, end + to.size() - begin);
    }
}

// The sound chips' generators: openMSX advances them as the mixer takes their samples, not as
// the machine runs, and a load starts the mixer afresh (MSXMotherBoard::serialize,
// ResampledSoundDevice::createResampler), so after a load they run up to a native sample apart
// from before. The machine never reads them; the sound check below compares what they play.
const char *const kSoundGenerators[] = {
    "toneGenerators", "noiseGenerator", "envelope", // AY8910 (PSG)
    "channels", "pm_phase", "am_phase", "noise_seed", // YM2413 (MSX-MUSIC)
};

// The machine's whole state, as a saved state holds it, without what the machine never reads:
// the time the file was written, the last time a stopped cassette player was synchronized
// (loading resynchronizes it, and it only counts while a tape plays), the mouse's last strobe
// time (loading plugs the mouse in again, which sets it; a game only notices within 1.5 ms of a
// strobe), and the sound generators.
bool state(MsxMachine &machine, const std::string &file, std::string &xml, std::string &error) {
    if (!machine.save_state(file, error)) return false;
    xml = unzip(read_file(file));
    if (xml.empty()) {
        error = "unreadable state " + file;
        return false;
    }
    cut(xml, 0, " date_time=\"", "\"");
    size_t cassette = xml.find("type=\"CassettePlayer\"");
    if (cassette != std::string::npos && xml.find("<state>STOP</state>", cassette) < xml.find("</pluggable>", cassette))
        cut(xml, cassette, "<prevSyncTime>", "</prevSyncTime>");
    size_t mouse = xml.find("type=\"Mouse\"");
    if (mouse != std::string::npos) cut(xml, mouse, "<lastTime>", "</lastTime>");
    for (const char *generator : kSoundGenerators) cut_all(xml, generator);
    return true;
}

// Whether two recordings play the same: the same length, give or take a sample, sound in the
// same frames (or the next or previous one, for a sound that starts or ends at a frame's edge),
// and the same loudness over all, give or take 5%. After a load the sound chips run up to a
// native sample apart (see kSoundGenerators): a noise channel then plays a different stretch of
// its random sequence, and the first frame fades in from the resamplers' silence.
bool same_sound(const std::vector<int16_t> &a, const std::vector<int16_t> &b, uint32_t frame_samples) {
    if (a.size() > b.size() + 2 || b.size() > a.size() + 2) {
        std::printf("sound: %zu and %zu samples\n", a.size() / 2, b.size() / 2);
        return false;
    }
    const size_t length = std::min(a.size(), b.size());
    // A frame quieter than 1/1000 of full scale is silent.
    const double silence = frame_samples * 2 * 33.0 * 33.0;
    std::vector<bool> audible_a, audible_b;
    double total_a = 0, total_b = 0;
    for (size_t start = 0; start < length; start += frame_samples * 2) {
        double energy_a = 0, energy_b = 0;
        for (size_t i = start; i < std::min(length, start + frame_samples * 2); ++i) {
            energy_a += double(a[i]) * a[i];
            energy_b += double(b[i]) * b[i];
        }
        audible_a.push_back(energy_a > silence);
        audible_b.push_back(energy_b > silence);
        total_a += energy_a;
        total_b += energy_b;
    }
    auto near = [](const std::vector<bool> &audible, size_t frame, bool value) {
        for (size_t f = frame ? frame - 1 : 0; f <= frame + 1 && f < audible.size(); ++f)
            if (audible[f] == value) return true;
        return false;
    };
    for (size_t frame = 0; frame < audible_a.size(); ++frame) {
        if (!near(audible_b, frame, audible_a[frame]) || !near(audible_a, frame, audible_b[frame])) {
            std::printf("sound: frame %zu is silent in only one\n", frame);
            return false;
        }
    }
    const double loudness_a = std::sqrt(total_a), loudness_b = std::sqrt(total_b);
    if (std::abs(loudness_a - loudness_b) > 0.05 * std::max(loudness_a, loudness_b)) {
        std::printf("sound: loudness %.0f and %.0f\n", loudness_a, loudness_b);
        return false;
    }
    return true;
}

void write_ppm(const std::string &file, const Run &run) {
    FILE *out = std::fopen(file.c_str(), "wb");
    if (!out) return;
    std::fprintf(out, "P6\n%u %u\n255\n", run.width, run.height);
    for (size_t i = 0; i < run.picture.size(); i += 4) std::fwrite(&run.picture[i], 1, 3, out);
    std::fclose(out);
}

void write_wav(const std::string &file, const std::vector<int16_t> &sound, uint32_t rate) {
    FILE *out = std::fopen(file.c_str(), "wb");
    if (!out) return;
    const uint32_t bytes = static_cast<uint32_t>(sound.size() * 2);
    auto u32 = [out](uint32_t v) { std::fwrite(&v, 4, 1, out); };
    auto u16 = [out](uint16_t v) { std::fwrite(&v, 2, 1, out); };
    std::fwrite("RIFF", 1, 4, out); u32(36 + bytes); std::fwrite("WAVEfmt ", 1, 8, out);
    u32(16); u16(1); u16(2); u32(rate); u32(rate * 4); u16(4); u16(16);
    std::fwrite("data", 1, 4, out); u32(bytes);
    std::fwrite(sound.data(), 2, sound.size(), out);
    std::fclose(out);
}

} // namespace

int main(int argc, char **argv) {
    Config config;
    // --machine <name> first picks the machine, by openMSX's name; C-BIOS MSX2+ otherwise.
    std::string probe;
    while (argc >= 3 && (std::string(argv[1]) == "--machine" || std::string(argv[1]) == "--command")) {
        (std::string(argv[1]) == "--machine" ? config.machine : probe) = argv[2];
        argc -= 2;
        argv += 2;
    }
    int key_row = -1, key_column = -1;
    if (argc == 6 && std::string(argv[5]).rfind("key:", 0) == 0 &&
        std::sscanf(argv[5] + 4, "%d,%d", &key_row, &key_column) != 2)
        return fail("a key is key:<row>,<column>");
    if (argc < 4 || argc > 6 ||
        (argc == 6 && std::string(argv[5]) != "joystick" && std::string(argv[5]) != "none" && key_row < 0))
        return fail("usage: boot_test [--machine <name>] <data dir> <work dir> <seconds> "
                    "[cartridge [joystick|none|key:<row>,<column>]]");
    const std::string work = argv[2];
    config.data_dir = argv[1];
    config.home = work + "/home";
    const double seconds = std::atof(argv[3]);
    if (argc >= 5) config.media[kairomsx::kCartA] = argv[4];
    const std::string pressing = argc == 6 ? argv[5] : "Space";

    std::string error;
    auto machine = MsxMachine::create(config, error);
    if (!machine) return fail("create: " + error);
    Run boot;
    if (!run(*machine, seconds, boot, error)) return fail("run: " + error);
    write_ppm(work + "/boot.ppm", boot);
    write_wav(work + "/boot.wav", boot.sound, static_cast<uint32_t>(machine->sample_rate()));
    std::printf("boot: %ux%u picture, %llu audio frames (%llu audible), %.2f guest seconds, %.4f s frames\n",
                boot.width, boot.height, (unsigned long long) boot.audio_frames, (unsigned long long) boot.loud_frames,
                boot.seconds, machine->frame_seconds());
    const double expected = boot.seconds * machine->sample_rate();
    if (boot.audio_frames < expected * 0.95 || boot.audio_frames > expected * 1.05)
        return fail("audio does not follow guest time");
    std::printf("cartridge A: %s\n", machine->media_path(kairomsx::kCartA).c_str());
    if (!probe.empty()) {
        std::string result;
        if (!machine->command(probe, result)) return fail("command: " + result);
        std::printf("command: %s\n", result.c_str());
    }

    Run pressed;
    auto press = [&](bool down) {
        if (pressing == "joystick") machine->joystick(kTriggerA, down);
        else if (pressing == "Space") machine->key(kSpaceRow, kSpaceColumn, down);
        else if (key_row >= 0) machine->key(key_row, key_column, down);
    };
    press(true);
    if (!run(*machine, 0.5, pressed, error)) return fail("run while pressed: " + error);
    press(false);
    if (!run(*machine, 4, pressed, error)) return fail("run after press: " + error);
    write_ppm(work + "/pressed.ppm", pressed);
    std::printf("press %s: the picture %s\n", pressing.c_str(),
                pressed.picture == boot.picture ? "did not change" : "changed");

    // The second after the save and the second after the load, frame by frame: state a saved state
    // misses shows up as states that drift apart.
    const std::string saved_file = work + "/state.oms";
    const std::string check_file = work + "/check.oms";
    const int frames = static_cast<int>(1 / machine->frame_seconds() + 0.5);
    std::vector<std::string> saved(frames + 1);
    if (!state(*machine, saved_file, saved[0], error)) return fail("save state: " + error);
    Run after_save, after_load, ignored;
    for (int frame = 1; frame <= frames; ++frame) {
        if (!run(*machine, kOneFrame, after_save, error)) return fail("run after save: " + error);
        if (!state(*machine, check_file, saved[frame], error)) return fail("state: " + error);
    }
    if (!run(*machine, 2, ignored, error)) return fail("run on: " + error);
    if (!machine->load_state(saved_file, error)) return fail("load state: " + error);
    int drift = -1;
    for (int frame = 0; frame <= frames; ++frame) {
        if (frame > 0 && !run(*machine, kOneFrame, after_load, error)) return fail("run after load: " + error);
        std::string loaded;
        if (!state(*machine, check_file, loaded, error)) return fail("state: " + error);
        if (drift < 0 && loaded != saved[frame]) {
            drift = frame;
            std::ofstream(work + "/drift_saved.xml", std::ios::binary) << saved[frame];
            std::ofstream(work + "/drift_loaded.xml", std::ios::binary) << loaded;
        }
    }
    if (drift >= 0) std::printf("state: drifts %d frames after the load (drift_saved.xml, drift_loaded.xml)\n", drift);
    write_ppm(work + "/after_save.ppm", after_save);
    write_ppm(work + "/after_load.ppm", after_load);
    write_wav(work + "/after_save.wav", after_save.sound, static_cast<uint32_t>(machine->sample_rate()));
    write_wav(work + "/after_load.wav", after_load.sound, static_cast<uint32_t>(machine->sample_rate()));
    const bool same_picture = after_save.picture == after_load.picture;
    const bool sound = same_sound(after_save.sound, after_load.sound,
                                  static_cast<uint32_t>(machine->sample_rate() * machine->frame_seconds()));
    std::printf("state: the picture of the second after load %s, its sound %s (%s sample for sample)\n",
                same_picture ? "matches" : "differs", sound ? "matches" : "differs",
                after_save.sound == after_load.sound ? "also" : "not");
    if (drift >= 0 || !same_picture || !sound) return fail("state round trip");
    // Every slot, as the app asks after a load; slots the machine lacks are empty.
    for (int slot = 0; slot < kairomsx::kSlots; ++slot)
        std::printf("slot %d after the load: '%s'\n", slot, machine->media_path(kairomsx::Slot(slot)).c_str());

    machine.reset();
    auto second = MsxMachine::create(config, error);
    if (!second) return fail("second machine: " + error);
    Run again;
    if (!run(*second, seconds, again, error)) return fail("second run: " + error);
    write_ppm(work + "/second.ppm", again);
    std::printf("second machine: %ux%u picture, %llu audio frames\n", again.width, again.height,
                (unsigned long long) again.audio_frames);
    if (again.picture != boot.picture) return fail("the second machine drew a different picture");
    std::printf("PASS\n");
    return 0;
}
