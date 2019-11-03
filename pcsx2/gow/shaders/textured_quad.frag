#version 330 core

in vec2 aTexCoord;

out vec4 oFragColor;

uniform sampler2D uTexture;

void main() {
	vec4 color = texture(uTexture, vec2(aTexCoord.x, 1.0 - aTexCoord.y));
	// oFragColor = vec4((color * (color.a * (255.0 / 128.0))).xyz, 1.0);
	oFragColor = vec4(color.xyz, color.a * (255.0 / 128.0));
}
