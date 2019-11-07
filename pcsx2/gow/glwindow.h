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
	u32 width, height;
    bool doUpdateViewport;

	void updateViewPort();
	void loadGl();

	void attachContext();
	void detachContext();

	friend class Context;
	class Context {
		bool detach;
		Window &w;
	public:
		Context(Window &w): w(w) {
			if (w.contextAttached) {
				detach = false;
			} else {
				w.attachContext();
				detach = true;
			}
		};
		~Context() { if (detach) { w.detachContext(); } };
	};
public:
	Window();
	~Window();

    void Create();
    void CreateContext(int major, int minor);

	Context AttachContext() { return Context(*this); }

    void Destroy();
	void SwapBuffers();
    void *GetProcAddress(char *name);
	void UpdateViewPort() { doUpdateViewport = true; };
	u32 GetWidth() { return width; };
    u32 GetHeight() { return height; };

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
};

} // namespace gow
