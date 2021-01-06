#version 330 core

in vec2 fTexCoord;

out vec4 oFragColor;

uniform sampler2D uTexture;

void main() {
	vec4 color = texture(uTexture, vec2(fTexCoord.x, 1.0 - fTexCoord.y));
	oFragColor = vec4(color.xyz, color.a * (255.0 / 128.0));
}
