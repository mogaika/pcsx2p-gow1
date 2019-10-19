#pragma once

#include "gow/glwindow.h"

namespace gow {

class Renderer {
	Window *window;
public:
	Renderer(Window *window);

	void CheckErrors(char *phase);

	void RenderFlashes();
	void EndOfFrame();
	void Setup();
};

} // namespace gow
