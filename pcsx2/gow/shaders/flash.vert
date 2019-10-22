#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

uniform mat4 uMatrix;
uniform float size1;
uniform float size2;

out vec2 outTexCoord;
out vec4 outColor;

void main() {
	outTexCoord = aTexCoord * (1.0/4096.0);
	outColor = aColor;
	// * 8 to complete fp16 conversion
    //gl_Position = uMatrix * vec4(aPos * (1.0 / 256.0), 1.0);
	// gl_Position = uMatrix * vec4(aPos * (1.0 / 32.0), 1.0);
	// pos = pos + vec3(uMatrix[3][0], uMatrix[3][1], uMatrix[3][2]), 1.0);


	vec4 pos = vec4(aPos, 1.0);
	
	mat4 matrix = uMatrix;
	// matrix[3][2] = 0;
	pos = pos * 0.0625; // normalize fixed point to floating point

	//pos = pos * size1;

	pos = vec4(pos.xyz, 1.0);

	pos = matrix * pos;

	// pos = pos * size2;
	// pos = vec4(pos.xy * vec2(0.0016, 0.00209*1.25*1.05), 1.0, 1.0);
	pos = vec4(pos.xy * vec2(0.0016, 0.00209), 0.0, 1.0);

	gl_Position = pos;
}
