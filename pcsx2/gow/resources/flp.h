#pragma once

#include "gow/utils.h"
#include "gow/resources/mesh.h"

namespace gow {

namespace raw {
	
#pragma pack(push, 1)

struct stFlpRawData2Sub1 {
	u8 blendColor[4];
	u32 txrInstanceOffset;
};
static_assert(sizeof(stFlpRawData2Sub1) == 0x8, "flpRawData2Sub1 size");

struct stFlpRawData2 {
	u16 meshPartIndex;
	u16 subsCount;
	u32 _astFlpRawData2Sub1;

	stFlpRawData2Sub1 *getData2Sub1(u32 index) { return &pmem<stFlpRawData2Sub1>(_astFlpRawData2Sub1)[index]; };
};
static_assert(sizeof(stFlpRawData2) == 0x8, "flpRawData2 size");

struct stModelInstance {
	gap_t _gap0[0xe0];
	u32 meshesCount;
	u32 _offsetMeshesArray;
	gap_t _gapEC[0x28];

	stMesh *getMesh(u32 index) { return pmem<stMesh>(pmem<u32>(_offsetMeshesArray)[index]);}
};
static_assert(sizeof(stModelInstance) == 0x110, "modelInstance size");

struct stFlpInstance {
    gap_t matrix1[0x40];
    gap_t matrix2[0x40];
    gap_t matrix3[0x40];
	gap_t _gapC0[0xc24];
	u32 _renModelInstanceOffset;
	gap_t _gapCE8[0x58];

	stModelInstance *getModel() { return pmem<stModelInstance>(_renModelInstanceOffset); }
};
static_assert(sizeof(stFlpInstance) == 0xd40, "flpInstance size");

#pragma pack(pop)

} // namespace raw

} // namespace gow
