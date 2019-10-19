#include "PrecompiledHeader.h"

#include "gow/renderer.h"
#include "gow/gl.h"
#include "gow/gow.h"

using namespace gow;

void gow::Renderer::loadShaders() {
    shader_textured_quad.Load("../../gow/shaders/textured_quad.vert", "../../gow/shaders/textured_quad.frag");
}

Renderer::Renderer(Window *window):
	window(window) {
	Setup();
}

void Renderer::CheckErrors(char *phase) {
    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        DevCon.Error("gow: OPENGL phase '%s' ERROR 0x%x %d", phase, error, error);
    }
}

void Renderer::Setup() {
	if (window->wglSwapIntervalEXT) {
		// window->wglSwapIntervalEXT(-1);
	}
	window->AttachContext();

	glClearColor(1.0, 1.0, 0, 1.0);
	CheckErrors("color");

	glClearDepth(10000.0);
	CheckErrors("setup");

	loadShaders();

	window->DetachContext();
}

void Renderer::renderTexturedQuad(GLuint texture) {
    static GLuint quad_vbo, quad_vao, quad_ebo;
    static float  vertices[] = {
        // positions        texture coords
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f   // top left
    };
    static unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
	static bool initialized = false;
	
	if (!initialized) {
        glGenVertexArrays(1, &quad_vao);
        glGenBuffers(1, &quad_vbo);
        glGenBuffers(1, &quad_ebo);

        glBindVertexArray(quad_vao);

		CheckErrors("gen bind buffers");

        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		CheckErrors("bind buffers");

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        // texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

		CheckErrors("enable vertex array");

		initialized = true;
	} 

	shader_textured_quad.Use();
    CheckErrors("use shader quad");

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(quad_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    CheckErrors("draw elements");
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
	CheckErrors("unuse program");
}

void Renderer::RenderFlashes() {
}

void Renderer::EndOfFrame() {
	window->AttachContext();

	// managers.texture->GetTexture
    CheckErrors("clear");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    CheckErrors("swap buffers");

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	auto texture = managers.texture->GetTexture(0x64a590);
	if (texture) {
		renderTexturedQuad(texture->GetGl(0));
        DevCon.WriteLn("has texture");
    } else {
        DevCon.WriteLn("no texture");
    }

    window->SwapBuffers();

	window->DetachContext();
}

GLuint Shader::compileShader(char *filename, GLuint shader) {
    wxFile file(filename);
    if (!file.IsOpened()) {
        DevCon.Error("gow: shader: can't open %d file %s", shader, filename);
        DevCon.Error(wxGetCwd());
		return shader;
    }
    
	wxString shaderString;
    file.ReadAll(&shaderString);
    file.Close();

	wxCStrData shaderCStr = shaderString.c_str();
	const char *shaderpChar = shaderCStr.AsChar();

    glShaderSource(shader, 1, &shaderpChar, NULL);
    glCompileShader(shader);

	GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
		char log[256];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        DevCon.Error("gow: shader: Error compiling %s", filename);
        DevCon.Error("%s", log);
    };

	return shader;
}

void Shader::Load(char *vertexFilename, char *fragmentFilename) {
    auto vert = glCreateShader(GL_VERTEX_SHADER);
    auto frag = glCreateShader(GL_FRAGMENT_SHADER);

	compileShader(vertexFilename, vert);
    compileShader(fragmentFilename, frag);

	program = glCreateProgram();
	glAttachShader(program, vert);
    glAttachShader(program, frag);
	glLinkProgram(program);

	GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[256];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        DevCon.Error("gow: shader: Error linking %s:%s", vertexFilename, fragmentFilename);
        DevCon.Error("%s", log);
    }

	glDeleteShader(vert);
    glDeleteShader(frag);
}

void gow::Shader::Use() {
	glUseProgram(program);
}
