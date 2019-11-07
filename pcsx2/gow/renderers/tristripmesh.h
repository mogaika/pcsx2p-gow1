#pragma once

#include "gow/gl.h"
#include "gow/resources/renderer.h"

namespace gow {

class Renderer;

namespace renderers {

class TriStripMesh {
protected:
	struct {;
		GLuint matrices;
		GLuint useTexture;
		GLuint blendColor;
		GLuint projectionMatrix;
		GLuint texYScale;
		GLuint texOffset;
		GLuint useJointIndexes;
	} uniforms;
public:
	TriStripMesh() {};

	void Setup(Renderer &r);
	void Render(Renderer &r, bool isDynamic, bool writeDepth, u32 groupIndexStart, u32 groupIndexEnd, u32 binsStructOffset);

};

} // namespace renderers

namespace offsets {
	static float &aspectRatio = pmemref<float>(0x2FD930);
} // namespace offsets
} // namespace gow
