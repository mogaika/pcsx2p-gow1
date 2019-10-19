#pragma once

#include <Windows.h>
#include <GL/GL.h>
#include <GL/wglext.h>

namespace gow {

class Window {
protected:
	HWND window;
    HDC display;
	HGLRC context;
    bool contextAttached;
	HMODULE glModule;
public:
	Window();
	~Window();

    void Create();
    void CreateContext(int major, int minor);
	void AttachContext();
	void DetachContext();
    void Destroy();
	void SwapBuffers();
    void *GetProcAddress(char *name);

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
};

} // namespace gow
