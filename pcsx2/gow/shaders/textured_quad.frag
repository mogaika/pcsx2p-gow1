#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D ourTexture;

void main() {
	vec4 clr = texture(ourTexture, vec2(texCoord.x, 1.0 - texCoord.y));
	// fragColor = vec4((clr * (clr.a * (255.0 / 128.0))).xyz, 1.0);
	fragColor = vec4(clr.xyz, clr.a * (255.0 / 128.0));
}
