#pragma once

#include "gow/utils.h"
#include "gow/resources/mesh.h"
#include "gow/resources/material.h"
#include "gow/resources/flp.h"

#include "../3rdparty/glm/mat4x4.hpp"

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

struct stRenderFlashTriStrip : stRenderFlashBase {
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
static_assert(sizeof(stRenderFlashTriStrip) == 0x18, "stRenderFlashTriStrip size");

struct stCViewport {
	glm::mat4 matrices[12];

	gap_t _gap0x000[0x68];
	float heightMultiplyer2;
	float heightMultiplyer1;
	float _unk0x370;
	float widthMultiplyer;
	gap_t _gap0x378[0x3c0 - 0x378];
};
static_assert(sizeof(stCViewport) == 0x3c0, "stCViewport size");

struct stViewportsStackElement {
	u32 _nextOffset, _prevOffset;
	u32 _viewportOffset;

	stViewportsStackElement *next() { return pmemz<stViewportsStackElement>(_nextOffset); }
	stViewportsStackElement *prev() { return pmemz<stViewportsStackElement>(_prevOffset); }
	stCViewport *viewport() { return pmemz<stCViewport>(_viewportOffset); }
};
static_assert(sizeof(stViewportsStackElement) == 0xc, "stViewportsStackElement size");

struct stViewportStackHead {
	u32 _headOffset;
	u32 _tailOffset;

	stViewportsStackElement *head() { return pmemz<stViewportsStackElement>(_headOffset); }
	stViewportsStackElement *tail() { return pmemz<stViewportsStackElement>(_tailOffset); }
	stViewportsStackElement *end() { return (stViewportsStackElement*) this; }
};
static_assert(sizeof(stViewportStackHead) == 0x8, "stViewportStackHead size");

#pragma pack(pop)

} // namespace raw

template <typename T>
class ps_pointer_wrapper_t {
protected:
	u32 offset;
public:
	ps_pointer_wrapper_t(u32 offset) : offset(offset) {};
	T *operator->() const { return *pmemz<T*>(offset) };
	operator bool() const { return pmemz<T>(offset) != 0; };
};

namespace offsets {
	static ps_pointer_wrapper_t<raw::stViewportStackHead> viewportsList(0x2CBC78);
}

} // namespace gow
