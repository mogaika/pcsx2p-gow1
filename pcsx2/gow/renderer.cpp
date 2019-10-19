#include "PrecompiledHeader.h"

#include "gow/renderer.h"

using namespace gow;

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

	glClearColor(255, 255, 0, 0);
	CheckErrors("color");

	glClearDepth(10000.0);
	CheckErrors("setup");

	window->DetachContext();
}

void Renderer::RenderFlashes() {
}

void Renderer::EndOfFrame() {
	window->AttachContext();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CheckErrors("clear");

	window->SwapBuffers();
    CheckErrors("swap buffers");

	window->DetachContext();
}
