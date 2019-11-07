#pragma once

#include "gow/utils.h"
#include "gow/gl.h"

namespace gow {
	class Renderer;
namespace renderers {

class TexturePreview {
protected:
    GLuint quad_vbo, quad_vao, quad_ebo;
	GLuint currentTexture;

    void initializeQuad();
public:
	TexturePreview();

	void Render(class gow::Renderer &r);
	void SetTextureGL(GLuint texture) { currentTexture = texture; }
	void SetTextureTXR(u32 textureKey, u32 imageIndex);
};

} // namespace renderers
} // namespace gow
