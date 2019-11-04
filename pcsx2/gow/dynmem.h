#pragma once

#include "gow/utils.h"


namespace gow {

namespace dynmem {

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable : 4200)

struct stStackAllocator {
	u32 magic;
    u32 _nextChunk;
	u32 _prevChunk;
	u32 _prevAllocator;
	u32 memStart;
	u32 usableMemStart;
	u32 usableMemEnd;
	gap_t _gap0x1C[0xc];
	char name[16];
};
static_assert(sizeof(stStackAllocator) == 0x38, "stStackAllocator size");

#pragma warning(pop)
#pragma pack(pop)

} // namespace raw


namespace offsets {
static stCStack<dynmem::stStackAllocator> &allocatorsStack = pmemref<stCStack<dynmem::stStackAllocator>>(0x2CB940);
}

} // namespace gow
