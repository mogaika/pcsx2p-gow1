#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aVertexColor;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec2 aJointIndexes;

uniform mat4 uMatrices[12];

uniform float size1;
uniform float size2;

out vec2 fTexCoord;
out vec4 fVertexColor;

void main() {
    fTexCoord = aTexCoord;
	fVertexColor = aVertexColor;

    gl_Position = vec4(aPos, 1.0);
}
