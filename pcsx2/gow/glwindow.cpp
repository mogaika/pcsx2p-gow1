#include "PrecompiledHeader.h"

#include "gow/gow.h"
#include "gow/glwindow.h"
#include "gow/gl.h"

#include <GL/glext.h>

#include "../plugins/GSdx/Renderers/OpenGL/PFN_GLLOADER_CPP.h"

using namespace gow;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_CLOSE:
        // ShowWindow(hWnd, SW_HIDE);
        // DestroyWindow(hWnd);
			return 0;
			break;
        case WM_SIZE:
            if (core && core->Window()) {
                core->Window()->UpdateViewPort();
            }
			return 0;
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

void Window::Create() {
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = _T("GOWRNDRWND");

    if (!GetClassInfo(wc.hInstance, wc.lpszClassName, &wc)) {
        if (!RegisterClass(&wc)) {
            DevCon.Error("Wasn't able to create GoW Renderer class");
            return;
        }
    }

    DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_BORDER | WS_VISIBLE;

    window = CreateWindow(wc.lpszClassName, _T("GoW"), style, -1, -1, 800, 600, NULL, NULL, wc.hInstance, (LPVOID)this);
    if (window == NULL) {
        DevCon.Error("Wasn't able to create GoW Renderer window");
        return;
    }

    GLuint PixelFormat;         // Holds The Results After Searching For A Match
    PIXELFORMATDESCRIPTOR pfd = // pfd Tells Windows How We Want Things To Be
        {
            sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
            1,                             // Version Number
            PFD_DRAW_TO_WINDOW |           // Format Must Support Window
                PFD_SUPPORT_OPENGL |       // Format Must Support OpenGL
                PFD_DOUBLEBUFFER,          // Must Support Double Buffering
            PFD_TYPE_RGBA,                 // Request An RGBA Format
            32,                            // Select Our Color Depth
            0, 0, 0, 0, 0, 0,              // Color Bits Ignored
            0,                             // 8bit Alpha Buffer
            0,                             // Shift Bit Ignored
            0,                             // No Accumulation Buffer
            0, 0, 0, 0,                    // Accumulation Bits Ignored
            0,                             // 24Bit Z-Buffer (Depth Buffer)
            8,                             // 8bit Stencil Buffer
            0,                             // No Auxiliary Buffer
            PFD_MAIN_PLANE,                // Main Drawing Layer
            0,                             // Reserved
            0, 0, 0                        // Layer Masks Ignored
        };

	display = GetDC(window);
	if (!display) {
		DevCon.Error("Can't create GL Device Context");
		return;
	}

    PixelFormat = ChoosePixelFormat(display, &pfd);
    if (!PixelFormat) {
        DevCon.Error("Can't Find A Suitable PixelFormat");
        return;
    }

    if (!SetPixelFormat(display, PixelFormat, &pfd)) {
        DevCon.Error("Can't Set The PixelFormat");
        return;
    }
}

void *Window::GetProcAddress(char *name) {
	PROC proc = wglGetProcAddress(name);
	if (proc) {
 		return proc;
	} else {
        if (!glModule) {
            DevCon.Error("Wasn't able to find %s wglproc: %d", name, GetLastError());
        }
    }

	if (!glModule) {
        glModule = LoadLibraryA("opengl32.dll");
        if (!glModule) {
            DevCon.Error("Wasn't able to load opengl32 lib: %d", GetLastError());
        }
    }

	auto glProc = ::GetProcAddress(glModule, name);
	if (!glProc) {
        DevCon.Error("Wasn't able to find %s glproc: %d", name, GetLastError());
	}
	return glProc;
}

void Window::CreateContext(int major, int minor) {
    context = wglCreateContext(display);
    if (!context) {
        DevCon.Error("Wasn't able to create gl context");
		return;
    }

	AttachContext();

	SetLastError(0);
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) GetProcAddress("wglCreateContextAttribsARB");
    if (!wglCreateContextAttribsARB) {
        DevCon.Error("Failed to init wglCreateContextAttribsARB function pointer");
        return;
    }

    int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, // | WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0};

    HGLRC context_new = wglCreateContextAttribsARB(display, NULL, context_attribs);
    if (!context_new) {
        DevCon.Error("Failed to create a 3.x context with standard flags");

        context_attribs[2 * 2 + 1] = 0;
        context_new = wglCreateContextAttribsARB(display, NULL, context_attribs);
    }

    DetachContext();

	if (!context_new) {
        DevCon.Error("Failed to create a 3.x context with compatible flags");
		return;
    }
    wglDeleteContext(context);

    context = context_new;

	AttachContext();

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) GetProcAddress("wglSwapIntervalEXT");
    wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) GetProcAddress("wglGetExtensionsStringARB");

	DevCon.WriteLn("gow: Opengl vendor: %s", glGetString(GL_VENDOR));
    DevCon.WriteLn("gow: Opengl renderer: %s", glGetString(GL_RENDERER));    

	DevCon.WriteLn("gow: loading opengl methods");    
	loadGl();
    DevCon.WriteLn("gow: loaded opengl methods");    

	updateViewPort();

	DetachContext();
}

void Window::updateViewPort() {
	RECT size;
    GetClientRect(window, &size);
    glViewport(0, 0, size.right, size.bottom);
	DevCon.WriteLn("size %d %d", size.right, size.bottom);
}

void Window::AttachContext() {
	if (!contextAttached) {
		if (!wglMakeCurrent(display, context)) {
            DevCon.Error("gow: Opengl: Can't attach to context: %d", GetLastError());
		}
        contextAttached = true;
    } else {
        DevCon.Error("gow: Opengl: Trying to attach when already attached");
	}
}

void Window::DetachContext() {
    if (contextAttached) {
        if (!wglMakeCurrent(NULL, NULL)) {
            DevCon.Error("gow: Opengl: Can't detach from context: %d", GetLastError());
        }
        contextAttached = false;
    } else {
        DevCon.Error("gow: Opengl: Trying to detach when already detached");
    }
}

void Window::Destroy() {
    DestroyWindow(window);
    window = NULL;
}

void Window::SwapBuffers() {
    ::SwapBuffers(display);
}

void gow::Window::loadGl() {
#define GL_EXT_LOAD_OPT(name) *(void **)&(name) = GetProcAddress(#name);
#include "../plugins/GSdx/Window/PFN_WND.h"
}

Window::Window():
	contextAttached(false),
	glModule(NULL) {
	Create();
    CreateContext(3, 3);
}

Window::~Window() { Destroy(); }
