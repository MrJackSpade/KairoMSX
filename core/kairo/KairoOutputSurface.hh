// The picture KairoMSX's host shows: the OutputSurface openMSX's display paints its video layers
// into, as RGBA8 pixels in memory. The PostProcessor paints the shown frame at its native size,
// so the surface's size follows the MSX's screen mode.

#ifndef KAIRO_OUTPUTSURFACE_HH
#define KAIRO_OUTPUTSURFACE_HH

#include "OutputSurface.hh"

#include <cstdint>
#include <span>
#include <vector>

namespace openmsx {

class KairoOutputSurface final : public OutputSurface
{
public:
	/** Sized for openMSX's default 320x240 picture until the first frame is painted. */
	KairoOutputSurface();

	/** Starts a new picture of width x height pixels and returns its rows to paint. */
	[[nodiscard]] std::span<Pixel> startPicture(unsigned width, unsigned height);

	[[nodiscard]] unsigned getWidth() const { return width; }
	[[nodiscard]] unsigned getHeight() const { return height; }
	/** RGBA8 rows of getWidth() pixels, without padding. */
	[[nodiscard]] const uint8_t* getPixels() const;

	/** How many pictures have been painted. */
	[[nodiscard]] uint64_t getPaintCount() const { return paintCount; }

	// OutputSurface
	void saveScreenshot(const std::string& filename) override;

private:
	void resize(unsigned width, unsigned height);

	unsigned width = 0;
	unsigned height = 0;
	std::vector<Pixel> pixels;
	uint64_t paintCount = 0;
};

} // namespace openmsx

#endif // KAIRO_OUTPUTSURFACE_HH
