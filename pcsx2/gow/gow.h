#pragma once

#include "gow/glwindow.h"
#include "gow/renderer.h"

#include "gow/resources/txr.h"
#include "gow/resources/mesh.h"

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
	MeshManager *mesh;
};

namespace offsets {
	static u32 &uFrameCounter = pmemref<u32>(0x32ED58);
}

extern gow::Core *core;
extern gow::ResourceManagers managers;

} // namespace gow
