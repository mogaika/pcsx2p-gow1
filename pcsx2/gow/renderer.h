#pragma once

#include "gow/glwindow.h"
#include "gow/utils.h"
#include "gow/resources/mesh.h"

namespace gow {

class Shader {
protected:
	GLuint program;
	GLuint compileShader(char *filename, GLuint shader);
	bool loaded;
public:
    Shader(): loaded(false) {};
	void Load(char *vertexFilename, char *fragmentFilename);
	void Use();
	GLuint GetUniformLocation(char *name);
};

#pragma pack(push, 1)

struct stFlpRawData2Sub1 {
	u32 blendColor;
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

struct stRenderFlashUIBase {
	u32 _nextOffset;
	u32 type;

	stRenderFlashUIBase *next() { return pmemz<stRenderFlashUIBase>(_nextOffset); }
};
static_assert(sizeof(stRenderFlashUIBase) == 0x8, "render flash base size");

struct stRenderFlashUISetColor : stRenderFlashUIBase {
	u16 color[4];
};
static_assert(sizeof(stRenderFlashUISetColor) == 0x10, "render flash color size");

struct stRenderFlashUIDraw : stRenderFlashUIBase {
	u32 _flpInstanceOffset;
	u32 _matrixOffset;
	u32 _stFlp2Offset;

	stFlpInstance *flpInstance() { return pmem<stFlpInstance>(_flpInstanceOffset); }
    GLfloat *matrix() { return pmem<GLfloat>(_matrixOffset); }
	stFlpRawData2 *flpRawData2() { return pmem<stFlpRawData2>(_stFlp2Offset); }
};
static_assert(sizeof(stRenderFlashUIDraw) == 0x14, "render flash draw size");

#pragma pack(pop)

class Renderer {
protected:
	Window *window;

	u32 currentPreviewTexture;
	bool reloadShadersRequest;

	void loadShaders();
    void renderTexturedQuad(GLuint texture);
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	void RenderFlashes();
	void EndOfFrame();
	void Setup();
    void ReloadShaders() { reloadShadersRequest = true; };
    void PreviewTextureQuad(u32 textureKey = 0) { currentPreviewTexture = textureKey; };
	
	Shader shader_textured_quad;
    Shader shader_flash;

	float size1;
	float size2;
};

} // namespace gow
