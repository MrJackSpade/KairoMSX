#include "KairoOutputSurface.hh"

#include "PNG.hh"

#include "xrange.hh"

#include <bit>

namespace openmsx {

KairoOutputSurface::KairoOutputSurface()
{
	resize(320, 240);
}

std::span<OutputSurface::Pixel> KairoOutputSurface::startPicture(unsigned width_, unsigned height_)
{
	if (width_ != width || height_ != height) resize(width_, height_);
	++paintCount;
	return pixels;
}

void KairoOutputSurface::resize(unsigned width_, unsigned height_)
{
	width = width_;
	height = height_;
	pixels.assign(size_t(width) * height, 0xFF000000); // opaque black
	gl::ivec2 size{int(width), int(height)};
	calculateViewPort(size, size);
}

const uint8_t* KairoOutputSurface::getPixels() const
{
	static_assert(std::endian::native == std::endian::little,
	              "openMSX's pixels are RGBA bytes on little-endian hosts");
	return std::bit_cast<const uint8_t*>(pixels.data());
}

void KairoOutputSurface::saveScreenshot(const std::string& filename)
{
	std::vector<const uint32_t*> rows(height);
	for (auto y : xrange(height)) rows[y] = &pixels[size_t(y) * width];
	PNG::saveRGBA(width, rows, filename);
}

} // namespace openmsx
