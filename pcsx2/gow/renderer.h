#pragma once

#include "gow/glwindow.h"
#include "gow/utils.h"
#include "gow/resources/mesh.h"
#include "gow/resources/flp.h"
#include "gow/resources/renderer.h"

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

class Renderer {
protected:
	Window *window;

	u32 currentPreviewTexture;
	bool reloadShadersRequest;
	bool dumpFrame;

	void loadShaders();
    void renderTexturedQuad(GLuint texture);
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	void RenderFlashes();
    void RenderStatic();

	void EndOfFrame();
	void Setup();
    void ReloadShaders() { reloadShadersRequest = true; };
    void PreviewTextureQuad(u32 textureKey = 0) { currentPreviewTexture = textureKey; };
    void DumpFrame() { dumpFrame = true; }
	
	Shader shader_textured_quad;
    Shader shader_flash;
    Shader shader_mesh;

	float size1;
	float size2;
};

} // namespace gow
