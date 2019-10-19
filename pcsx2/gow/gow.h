#pragma once

#include "gow/glwindow.h"
#include "gow/renderer.h"

namespace gow {

class Core {
protected:
	Window *window;
	Renderer *renderer;
public:
    Core();
    ~Core();

	void BeginOfFrame();
	void EndOfFrame();
};

} // namespace gow
