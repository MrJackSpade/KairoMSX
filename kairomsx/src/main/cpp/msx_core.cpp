// The MSX machine (core/msx_machine.h) as the MachineCore the shared frame host runs
// (shared/frontend/src/main/cpp/kairo/machine_core.h).

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "kairo/machine_core.h"
#include "msx_machine.h"

namespace {

using kairomsx::MsxMachine;

// Media slots, as MainActivity numbers them: the core's slots.
static_assert(kairomsx::kCartA == 0 && kairomsx::kCartB == 1 && kairomsx::kDiskA == 2 && kairomsx::kDiskB == 3 &&
              kairomsx::kCassette == 4);
// MainActivity.machine.stateFile.
constexpr const char *kStateFile = "/state.oms";
// MainActivity.stateError reads these.
// MsxKeys numbers a key by its matrix place: row * 8 + column + 1, rows 0 to 10.
constexpr int kKeyCodes = 11 * 8 + 1;

class MsxCore : public kairo::MachineCore {
public:
    explicit MsxCore(std::unique_ptr<MsxMachine> machine) : machine_(std::move(machine)) {}

    bool run_frame() override { return machine_->run_frame(error_); }
    std::string error() override { return error_; }
    // The emulator's reason, which save_state and load_state leave in error_.
    std::string state_error() override { return error_; }
    // Bit 0 Caps, as NativeMachine lists it (MainActivity); bit 1, Kana, waits for its key (KairoMSX#45).
    int locks() override { return machine_->locks(); }
    // Flat out while a disk or tape loads (Kairo#82).
    bool loading() override { return machine_->loading(); }
    const uint8_t *frame(uint32_t &width, uint32_t &height) override { return machine_->frame(width, height); }
    double frame_seconds() override { return machine_->frame_seconds(); }
    int32_t audio_rate() override { return machine_->sample_rate(); }
    uint32_t read_audio(int16_t *stereo, uint32_t max_frames) override { return machine_->read_audio(stereo, max_frames); }

    void release_inputs() override {
        for (int code = 1; code < kKeyCodes; ++code)
            if (keys_[code]) key(code, false);
        for (int control = 0; control < kairomsx::kJoystickControls; ++control) machine_->joystick(control, false);
        machine_->mouse_button(1, false);
        machine_->mouse_button(2, false);
    }

    void key(int code, bool down) override {
        if (code <= 0 || code >= kKeyCodes || keys_[code] == down) return;
        keys_[code] = down;
        machine_->key((code - 1) / 8, (code - 1) % 8, down);
    }

    // Controls in MsxKeys.joystick order, which is the core's.
    void joystick(int control, bool down) override { machine_->joystick(control, down); }
    // The mouse in port B.
    void mouse_move(int dx, int dy) override { machine_->mouse_move(dx, dy); }
    void mouse_button(int button, bool down) override { machine_->mouse_button(button, down); }

    void reset() override { machine_->reset(); }

    int save_state(const std::string &directory) override {
        return machine_->save_state(directory + kStateFile, error_) ? 0 : kFailed;
    }

    int load_state(const std::string &directory) override {
        return machine_->load_state(directory + kStateFile, error_) ? 0 : kFailed;
    }

    int media(int slot, const std::string &path) override {
        if (slot < 0 || slot >= kairomsx::kSlots) return kUnsupported;
        return machine_->insert(kairomsx::Slot(slot), path, error_) ? 0 : kFailed;
    }

    std::vector<std::string> media_paths() override {
        std::vector<std::string> paths;
        for (int slot = 0; slot < kairomsx::kSlots; ++slot) paths.push_back(machine_->media_path(kairomsx::Slot(slot)));
        return paths;
    }

private:
    std::unique_ptr<MsxMachine> machine_;
    std::string error_;
    std::array<bool, kKeyCodes> keys_{};
};

} // namespace

// The launch's home holds openMSX's folders, as MainActivity prepares them: "system", the data
// folder the app's assets/openmsx is copied to, and "user", openMSX's own files.
std::unique_ptr<kairo::MachineCore> kairo::create_core(const kairo::Launch &launch, std::string &error) {
    kairomsx::Config config;
    config.data_dir = launch.home + "/system";
    config.home = launch.home + "/user";
    for (int slot = 0; slot < kairomsx::kSlots; ++slot)
        if (int(launch.media.size()) > slot) config.media[slot] = launch.media[slot];
    // The settings MsxCatalog's options name; every launch passes them.
    config.machine = launch.settings.at("machine");
    config.memory_kb = std::stoi(launch.settings.at("memory"));
    const std::string &clock = launch.settings.at("clock");
    config.z80_hz = clock == "auto" ? 0 : std::stoi(clock);
    auto machine = MsxMachine::create(config, error);
    if (!machine) return nullptr;
    return std::make_unique<MsxCore>(std::move(machine));
}
