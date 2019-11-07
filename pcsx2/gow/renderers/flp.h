#pragma once

#include "gow/resources/renderer.h"

namespace gow {

class Renderer;

namespace renderers {

class Flp {
protected:
	struct {;
		GLuint matrix;
		GLuint useTexture;
		GLuint blendColor;
		GLuint texYScale;
	} uniforms;
public:
	Flp() {};

	void Setup(Renderer &r);
	void Render(Renderer &r, raw::stRenderFlashUIBase *firstFlash);
};

} // namespace renderers
} // namespace gow
