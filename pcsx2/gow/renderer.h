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

	void loadShaders();
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	void RenderFlashes();
	void EndOfFrame();
	void Setup();
	void renderTexturedQuad(GLuint texture);
	
	Shader shader_textured_quad;
};

} // namespace gow
