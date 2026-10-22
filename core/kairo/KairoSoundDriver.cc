#include "KairoSoundDriver.hh"

#include "KairoHost.hh"

#include <algorithm>
#include <cmath>

namespace openmsx {

KairoSoundDriver::KairoSoundDriver(Reactor& /*reactor*/, unsigned /*wantedFreq*/, unsigned samples_)
	: host(KairoHost::current())
	, samples(samples_)
{
}

void KairoSoundDriver::mute()
{
	muted = true;
}

void KairoSoundDriver::unmute()
{
	muted = false;
}

unsigned KairoSoundDriver::getFrequency() const
{
	return host.sampleRate;
}

unsigned KairoSoundDriver::getSamples() const
{
	return samples;
}

static int16_t toSample(float value)
{
	return int16_t(std::lround(std::clamp(value, -1.0f, 1.0f) * 32767.0f));
}

void KairoSoundDriver::uploadBuffer(std::span<const StereoFloat> buffer)
{
	if (muted) return;
	for (const auto& s : buffer) {
		host.audio.push_back(toSample(s.left));
		host.audio.push_back(toSample(s.right));
	}
}

} // namespace openmsx
