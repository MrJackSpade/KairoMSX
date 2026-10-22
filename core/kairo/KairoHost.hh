// What connects openMSX's video and sound to KairoMSX's host. The machine that runs openMSX on a
// thread installs a KairoHost there for as long as openMSX lives (KairoHost::Install), and the
// video system and sound driver openMSX creates on that thread attach to it: the picture is the
// video system's KairoOutputSurface, and the sound driver queues the mixed sound here.

#ifndef KAIRO_HOST_HH
#define KAIRO_HOST_HH

#include <cstdint>
#include <deque>

namespace openmsx {

class KairoOutputSurface;

struct KairoHost
{
	/** The thread's host; fails when none is installed. */
	[[nodiscard]] static KairoHost& current();

	class Install {
	public:
		explicit Install(KairoHost& host);
		~Install();
		Install(const Install&) = delete;
		Install& operator=(const Install&) = delete;
	};

	/** The sample rate openMSX mixes at. */
	unsigned sampleRate = 48000;
	/** Interleaved stereo 16-bit samples mixed so far and not yet taken. */
	std::deque<int16_t> audio;
	/** The video system's surface; null while openMSX has none. */
	KairoOutputSurface* surface = nullptr;
};

} // namespace openmsx

#endif // KAIRO_HOST_HH
