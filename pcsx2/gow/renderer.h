#pragma once

#include "gow/gl.h"
#include "gow/glwindow.h"
#include "gow/utils.h"

namespace gow {
	class Renderer;
}

#include "gow/renderers/texturepreview.h"
#include "gow/renderers/master.h"

namespace gow {

class Shader {
protected:
	GLuint program;
	GLuint compileShader(char *filename, GLuint shader);
	bool loaded;
	int warnedTimes;
public:
    Shader(): loaded(false) {};
	void Load(char *vertexFilename, char *fragmentFilename);
	void Use();
	GLuint GetUniformLocation(char *name);
};

class Renderer {
protected:
	Window *window;
	bool reloadShadersRequest;
	bool dumpFrame;
	bool dumpNextFrame;
	wxString dumpPrefix;

	void loadShaders();

	friend class DumpContext;
	class DumpContext {
	protected:
		wxString savedPrefix;
		Renderer &r;
	public:
		DumpContext(Renderer &r, char *suffix):
			r(r) {
			savedPrefix = r.dumpPrefix;
			r.dumpPrefix = r.dumpPrefix.Append(suffix).Append(": ");
		};
		~DumpContext() { r.dumpPrefix = savedPrefix; };
		void Log(ConsoleColors color, const char *format, ...);
		void LogMatrix(ConsoleColors color, glm::mat4 &matrix);
	};
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	Window &Window() { return *window; };

	void EndOfFrame();
	void Setup();
    void ReloadShaders() { reloadShadersRequest = true; };
    void DumpFrame() { dumpNextFrame = true; }
	bool IsDumpingFrame() { return dumpFrame; }
	DumpContext LogDumpPush(char *suffix) { return DumpContext(*this, suffix); }
	
	Shader shader_textured_quad;
    Shader shader_flash;
    Shader shader_mesh;

	renderers::TexturePreview TexturePreview;
	renderers::Master Master;
};

} // namespace gow
