#pragma once

#include "gow/utils.h"

#include "gow/resources/txr.h"

#include "../3rdparty/glm/vec2.hpp"
#include "../3rdparty/glm/vec4.hpp"

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
	u32 flags[4];
	gap_t _gap0x10[0x4];
	glm::vec4 blendColor;
	gap_t _gap0x24[0x14];
	u32 _textureOffset;
	gap_t _gap0x3C[0x48-0x3C];
	glm::tvec2<s32> offset;
	gap_t _gap0x50[0xB0-0x50];

	stTxr *texture() { return pmemz<stTxr>(_textureOffset); }
	bool haveTexture() { return (flags[0] >> 7) & 1;}

	bool isDisableDepthWrite() { return (flags[0] >> 19) & 1; }
    bool isBlendReflection() { return (flags[0] >> 24) & 1; }
    bool isBlendSubstract() { return (flags[0] >> 25) & 1; }
    bool isBlendNone() { return (flags[0] >> 26) & 1; }
    bool isBlendAdditive() { return (flags[0] >> 27) & 1; }
};
static_assert(sizeof(stMatLayer) == 0xB0, "stMatLayer size");

#pragma warning(pop)
#pragma pack(pop)

} // namespace raw

} // namespace gow
