#version 330 core

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aVertexColor;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec2 aJointIndexes;

uniform mat4 uMatrices[12];
uniform mat4 uProjectionMatrix;
uniform float uTexYScale;
uniform vec2 uTexOffset;
uniform bool uUseJointIndexes;

uniform float size1;
uniform float size2;

out vec2 fTexCoord;
out vec4 fVertexColor;

void main() {
 	vec2 texCoord = (aTexCoord + uTexOffset) * (1.0/4096.0);
	fTexCoord = vec2(texCoord.x, texCoord.y * uTexYScale);

	fVertexColor = aVertexColor;

	vec4 pos = vec4(aPos.xyz * (1.0/16.0), 1.0); // normalize fixed point to floating point
	
	if (uUseJointIndexes) {
		mat4 matrix1 = uProjectionMatrix * uMatrices[int(aJointIndexes.x)];
		mat4 matrix2 = uProjectionMatrix * uMatrices[int(aJointIndexes.y)];

		float weight = aPos.w;
		if (weight < 0.0) {
			weight += 32768.0;
		}
		weight *= (1.0/4096.0);

		pos = (matrix2 * weight + matrix1 * (1.0 - weight)) * pos;
	} else {
		mat4 matrix = uProjectionMatrix * uMatrices[0];
		pos = matrix * pos;
	}

	pos = vec4(pos.x, -pos.y, pos.zw);
	gl_Position = pos;
}
