#version 330 core

in vec2 fTexCoord;
in vec4 fVertexColor;

out vec4 oFragColor;

uniform bool uUseTexture;
uniform vec4 uBlendColor;
uniform sampler2D uTexture;

uniform float size1;
uniform float size2;

void main() {
	vec4 color;
	if (uUseTexture) {
		color = texture(uTexture, vec2(fTexCoord.x, fTexCoord.y));
		color = vec4(color.xyz, color.a * (255.0 / 128.0));
	} else {
		color = vec4(1.0, 1.0, 1.0, 1.0);
	}

	color = color * vec4(fVertexColor.xyz, fVertexColor.a * (255.0 / 128.0));

	color = color * uBlendColor; 

	// color = vec4(mod(fTexCoord.xy * 4, 1.0), 0.0, color.a);

	oFragColor = color;
}
