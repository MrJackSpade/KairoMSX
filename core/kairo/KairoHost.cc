#include "KairoHost.hh"

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace openmsx {

static thread_local KairoHost* installed = nullptr;

KairoHost& KairoHost::current()
{
	if (!installed) {
		std::fputs("KairoMSX: openMSX ran on a thread without a KairoHost\n", stderr);
		std::abort();
	}
	return *installed;
}

KairoHost::Install::Install(KairoHost& host)
{
	assert(!installed);
	installed = &host;
}

KairoHost::Install::~Install()
{
	installed = nullptr;
}

} // namespace openmsx
