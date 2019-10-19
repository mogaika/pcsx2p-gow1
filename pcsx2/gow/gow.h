#pragma once

#include "gow/glwindow.h"
#include "gow/renderer.h"

#include "gow/resources/txr.h"

namespace gow {

class Core {
protected:
	Window *window;
	Renderer *renderer;
public:
    Core();
    ~Core();

	Window *Window() { return window; }
    Renderer *Renderer() { return renderer; }
	void BeginOfFrame();
	void EndOfFrame();
};

struct ResourceManagers {
public:
    void Init();

	TextureManager *texture;
};

extern gow::Core *core;
extern gow::ResourceManagers managers;

} // namespace gow