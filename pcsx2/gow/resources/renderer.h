#pragma once

#include "gow/utils.h"
#include "gow/resources/mesh.h"
#include "gow/resources/material.h"

namespace gow {

namespace raw {
	
#pragma pack(push, 1)

struct stRenderFlashBase {
    u32 _nextOffset;

	stRenderFlashBase *next() { return pmemz<stRenderFlashBase>(_nextOffset); }
};
static_assert(sizeof(stRenderFlashBase) == 0x4, "stRenderFlashBase size");

struct stRenderFlashUIBase: stRenderFlashBase {
	u32 type;
};
static_assert(sizeof(stRenderFlashUIBase) == 0x8, "stRenderFlashUIBase size");

struct stRenderFlashUISetColor : stRenderFlashUIBase {
	u16 color[4];
};
static_assert(sizeof(stRenderFlashUISetColor) == 0x10, "stRenderFlashUISetColor size");

struct stRenderFlashUIDraw : stRenderFlashUIBase {
	u32 _flpInstanceOffset;
	u32 _matrixOffset;
	u32 _stFlp2Offset;

	stFlpInstance *flpInstance() { return pmem<stFlpInstance>(_flpInstanceOffset); }
    GLfloat *matrix() { return pmem<GLfloat>(_matrixOffset); }
	stFlpRawData2 *flpRawData2() { return pmem<stFlpRawData2>(_stFlp2Offset); }
};
static_assert(sizeof(stRenderFlashUIDraw) == 0x14, "stRenderFlashUIDraw size");

struct stRenderFlashStatic : stRenderFlashBase {
	u32 _meshObject;
	u32 _someBufferInStackForFlashesWithMatrices;
	u32 _usedMatLayer;
	float transparency;
	u8 instanceId;
	u8 layerIndex;
	u8 _boolIsStatic_or_unAnimated;
	gap_t _gap17[1];

	stMeshObject *meshObject() { return pmem<stMeshObject>(_meshObject); }
	stMatLayer *matLayer() { return pmemz<stMatLayer>(_usedMatLayer); }
};
static_assert(sizeof(stRenderFlashStatic) == 0x18, "stRenderFlashStatic size");

#pragma pack(pop)

} // namespace raw

} // namespace gow
