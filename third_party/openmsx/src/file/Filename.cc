#include "Filename.hh"
#include "MSXException.hh"
#include "serialize.hh"
#include <cassert>

// KairoMSX: where a file a save state names is now (shared kairo/state_paths.h).
extern "C" const char *kairo_state_path(const char *recorded);

namespace openmsx {

void Filename::updateAfterLoadState()
{
	if (empty()) return;
	// KairoMSX: a working copy can have another name since the state was saved.
	if (std::string now = kairo_state_path(resolvedFilename.c_str()); now != resolvedFilename) {
		originalFilename = now;
		resolvedFilename = std::move(now);
		return;
	}
	if (FileOperations::exists(resolvedFilename)) return;

	try {
		resolvedFilename = FileOperations::getAbsolutePath(
			userFileContext().resolve(originalFilename));
	} catch (MSXException&) {
		// nothing
	}
}

bool Filename::empty() const
{
	assert(getOriginal().empty() == getResolved().empty());
	return getOriginal().empty();
}

template<typename Archive>
void Filename::serialize(Archive& ar, unsigned /*version*/)
{
	ar.serialize("original", originalFilename,
	             "resolved", resolvedFilename);
}
INSTANTIATE_SERIALIZE_METHODS(Filename);

} // namespace openmsx
