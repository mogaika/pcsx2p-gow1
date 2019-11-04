#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aVertexColor;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec2 aJointIndexes;

uniform mat4 uMatrices[12];
uniform mat4 uProjectionMatrix;
uniform float uTexYScale;

uniform float size1;
uniform float size2;

out vec2 fTexCoord;
out vec4 fVertexColor;

void main() {
 	fTexCoord = aTexCoord * (1.0/4096.0);
	fTexCoord = vec2(fTexCoord.x, fTexCoord.y * uTexYScale);

	fVertexColor = aVertexColor;

	vec4 pos = vec4(aPos, 1.0);
	pos = pos * (1.0/16.0); // normalize fixed point to floating point
	// pos = pos * size1;

	mat4 matrix = uProjectionMatrix * uMatrices[0];
	// matrix = uProjectionMatrix;

	pos = vec4(pos.xyz, 1.0);

	// pos = pos * size2;

	pos = matrix * pos;

	gl_Position = pos;
}
