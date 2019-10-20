#pragma once

#include "gow/glwindow.h"

namespace gow {

class Shader {
protected:
	GLuint program;
	GLuint compileShader(char *filename, GLuint shader);
public:
	void Load(char *vertexFilename, char *fragmentFilename);
	void Use();
};

class Renderer {
protected:
	Window *window;

	u32 currentPreviewTexture;

	void loadShaders();
    void renderTexturedQuad(GLuint texture);
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	void RenderFlashes();
	void EndOfFrame();
	void Setup();
    void PreviewTextureQuad(u32 textureKey = 0) { currentPreviewTexture = textureKey; };
	
	Shader shader_textured_quad;
};

} // namespace gow
