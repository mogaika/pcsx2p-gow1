#include "PrecompiledHeader.h"

#include "gow/renderers/texturepreview.h"
#include "gow/renderer.h"
#include "gow/gow.h"

using namespace gow;

renderers::TexturePreview::TexturePreview(class gow::Renderer &r):
	currentTexture(0), renderer(r) {
	initializeQuad();
}

void renderers::TexturePreview::SetTextureTXR(u32 textureKey, u32 imageIndex) {
    auto texture = managers.texture->GetTexture(textureKey);
    if (texture) {
		SetTextureGL(texture->GetGl(imageIndex));
    } else {
		SetTextureGL(0);
        DevCon.Error("TexturePreview: Wasn't able to find texture 0x%x", textureKey);
    }
}

void renderers::TexturePreview::initializeQuad() {
	auto context = renderer.Window().AttachContext();

    static float vertices[] = {
        //    positions     texture coords
        0.5f,   0.5f, 0.0f, 1.0f, 1.0f,   // top right
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  // top left
    };
    static unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3, // second triangle
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glGenBuffers(1, &quad_ebo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void renderers::TexturePreview::Render() {
	if (currentTexture == 0) {
		return;
	}

	auto log = renderer.LogDumpPush("TexturePreview");

	auto glContext = renderer.Window().AttachContext();
	renderer.shader_textured_quad.Use();
	renderer.CheckErrors("use shader quad");

	glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentTexture);

	glBindVertexArray(quad_vao);
	renderer.CheckErrors("glBindVertexArray");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
	renderer.CheckErrors("glBindBuffer");
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	renderer.CheckErrors("draw elements");
}