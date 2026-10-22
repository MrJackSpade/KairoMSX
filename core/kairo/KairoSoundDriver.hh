// openMSX's sound driver for KairoMSX: queues the mixed sound for the host (KairoHost::audio)
// instead of playing it. sound/Mixer.cc creates it in place of SDLSoundDriver in KairoMSX's
// build (tools/local_patches.py).

#ifndef KAIRO_SOUNDDRIVER_HH
#define KAIRO_SOUNDDRIVER_HH

#include "SoundDriver.hh"

namespace openmsx {

class Reactor;
struct KairoHost;

class KairoSoundDriver final : public SoundDriver
{
public:
	/** The host's sample rate replaces the wanted one. */
	KairoSoundDriver(Reactor& reactor, unsigned wantedFreq, unsigned samples);

	void mute() override;
	void unmute() override;

	[[nodiscard]] unsigned getFrequency() const override;
	[[nodiscard]] unsigned getSamples() const override;

	void uploadBuffer(std::span<const StereoFloat> buffer) override;

private:
	KairoHost& host;
	unsigned samples;
	bool muted = true;
};

} // namespace openmsx

#endif // KAIRO_SOUNDDRIVER_HH
