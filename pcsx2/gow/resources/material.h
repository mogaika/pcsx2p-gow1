#pragma once

#include "gow/utils.h"

#include "gow/resources/txr.h"

#include "../3rdparty/glm/vec2.hpp"

namespace gow
{

class Mesh;
class MeshObject;

namespace raw
{

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable : 4200)

struct stMatLayer {
    gap_t _gap0x0[0x38];
	u32 _textureOffset;
	gap_t _gap0x3C[0x48-0x3C];
	glm::tvec2<u32> offset;
	gap_t _gap0x50[0xB0-0x50];

	stTxr *texture() { return pmemz<stTxr>(_textureOffset); }
};
static_assert(sizeof(stMatLayer) == 0xB0, "stMatLayer size");

#pragma warning(pop)
#pragma pack(pop)

} // namespace raw

} // namespace gow
