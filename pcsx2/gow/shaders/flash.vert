#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uMatrix;
uniform float uTexYScale;

uniform float size1;
uniform float size2;

out vec2 fTexCoord;

void main() {
	fTexCoord = aTexCoord * (1.0/4096.0);
	fTexCoord = vec2(fTexCoord.x, fTexCoord.y * uTexYScale);

	vec4 pos = vec4(aPos, 1.0);
	
	mat4 matrix = uMatrix;
	
	pos = pos * (1.0/16.0); // normalize fixed point to floating point

	//pos = pos * size1;

	pos = vec4(pos.xyz, 1.0);

	pos = matrix * pos;

	// pos = pos * size2;
	// TODO: these constans can vary, detect where viewport size is setted
	pos = vec4(pos.xy * vec2(0.0016, 0.00209), 0.0, 1.0);

	gl_Position = pos;
}
