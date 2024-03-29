#version 330 core

in vec2 fTexCoord;

out vec4 oFragColor;

uniform bool uUseTexture;
uniform vec4 uBlendColor;
uniform sampler2D uTexture;

uniform float size1;
uniform float size2;

void main() {
	vec4 color;

	color = uBlendColor;

	if (uUseTexture) {
		vec4 textureColor = texture(uTexture, vec2(fTexCoord.x, fTexCoord.y));
		color *= vec4(textureColor.xyz, textureColor.a * (255.0 / 128.0));
	}

	oFragColor = color;
}
