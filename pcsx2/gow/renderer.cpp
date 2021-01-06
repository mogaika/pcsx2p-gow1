#include "PrecompiledHeader.h"

#include "gow/gl.h"
#include "gow/glwindow.h"
#include "gow/gow.h"

#include "gow/resources/mesh.h"
#include "gow/resources/renderer.h"

#include "../3rdparty/glm/mat4x4.hpp"
#include "../3rdparty/glm/ext/matrix_clip_space.hpp"
#include "../3rdparty/glm/ext/scalar_constants.hpp"
#include "../3rdparty/glm/gtc/type_ptr.hpp"


using namespace gow;

void gow::Renderer::loadShaders() {
    shader_textured_quad.Load("../../gow/shaders/textured_quad.vert", "../../gow/shaders/textured_quad.frag");
	shader_flash.Load("../../gow/shaders/flash.vert", "../../gow/shaders/flash.frag");
    shader_mesh.Load("../../gow/shaders/mesh.vert", "../../gow/shaders/mesh.frag");

	Master.Setup();
}

Renderer::Renderer(gow::Window *window):
	window(window), 
	reloadShadersRequest(0),
	dumpFrame(false),
	dumpNextFrame(false),
	Master(*this),
	TexturePreview(*this) {
	Setup();
	auto glContext = window->AttachContext();
	Master.FrameBegin();
}

void Renderer::CheckErrors(char *phase) {
    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        DevCon.Error("GOW:%s OPENGL phase '%s' ERROR 0x%x %d", dumpPrefix.c_str().AsChar(), phase, error, error);
    }
}

void Renderer::Setup() {
	auto glContext = window->AttachContext();

	if (window->wglSwapIntervalEXT) {
		window->wglSwapIntervalEXT(-1);
	}

	CheckErrors("setup");
	loadShaders();
}

void Renderer::EndOfFrame() {
	auto glContext = window->AttachContext();

	Master.FrameEnd();

	TexturePreview.Render();

	LogDumpPush("CORE").Log(Color_Blue, "#### ========= DUMPED FRAME %d ========= ####", offsets::uFrameCounter);

	if (reloadShadersRequest) {
		loadShaders();
	}
	reloadShadersRequest = false;

    dumpFrame = dumpNextFrame;
    dumpNextFrame = false;

    window->SwapBuffers();
    CheckErrors("swap buffers");

	Master.FrameBegin();
}

void Renderer::DumpContext::Log(ConsoleColors color, const char *format, ...) {
	if (!r.dumpFrame) {
		return;
	}

	va_list args;
	va_start(args, format);
	wxString line = r.dumpPrefix + wxString::FormatV(format, args);
	va_end(args);

	DevCon.WriteLn(color, "%s", line.c_str().AsChar());
}

void Renderer::DumpContext::LogMatrix(ConsoleColors color, glm::mat4 &matrix) {
	for (int i = 0; i < 4; i++) {
		glm::vec4 &row = matrix[i];
		Log(color, "    [%d] %f %f %f %f", i, row.x, row.y, row.z, row.w);
	}
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

	GLuint oldProgram = program;
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

		glDeleteProgram(program);
		if (loaded) {
            program = oldProgram;
        }
    } else {
		if (loaded) {
            glDeleteProgram(oldProgram);
        }
		DevCon.WriteLn(Color_Yellow, "Shader reloaded (%s)(%s)", vertexFilename, fragmentFilename);
        loaded = true;
	}

	warnedTimes = 0;

	glDeleteShader(vert);
    glDeleteShader(frag);
}

void gow::Shader::Use() {
	glUseProgram(program);
}

GLuint Shader::GetUniformLocation(char *name) {
    GLuint loc = glGetUniformLocation(program, name);
	if (loc == -1) {
        if (warnedTimes < 16) {
			warnedTimes++;
			DevCon.Error("Wasn't able to get '%s' uniform", name);
        }
	}
	return loc;
}

