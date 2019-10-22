#version 330 core

in vec2 outTexCoord;
in vec4 outColor;

out vec4 fragColor;

uniform bool uUseTexture;
uniform vec4 uBlendColor;
uniform sampler2D ourTexture;

void main() {
	vec4 color;
	if (uUseTexture) {
		color = texture(ourTexture, vec2(outTexCoord.x, outTexCoord.y));
		color = vec4(color.xyz, color.a * (255.0 / 128.0));
	} else {
		color = vec4(1.0, 1.0, 1.0, 1.0);
	}
	// color = vec4(1.0, 1.0, 1.0, 1.0);

	color = color * uBlendColor; 
	// color = vec4(mod(outTexCoord.xy * 4, 1.0), 0.0, color.a);

	fragColor = color;
}
