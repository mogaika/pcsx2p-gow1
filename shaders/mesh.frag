#version 330 core

in vec2 fTexCoord;
in vec4 fVertexColor;

out vec4 oFragColor;

uniform bool uUseTexture;
uniform vec4 uBlendColor;
uniform sampler2D uTexture;
uniform float uTransparency;

uniform float flashId;
uniform float size1;
uniform float size2;

void main() {
	vec4 color;

	color.xyz = fVertexColor.xyz / 128.0;
	color.a = fVertexColor.a / 128.0;

	if (uUseTexture) {
		vec4 textureColor = texture(uTexture, vec2(fTexCoord.x, fTexCoord.y));
		color *= vec4(textureColor.xyz, textureColor.a * (255.0 / 128.0));
	}

	//color.rgb *= color.a;
	//color.a = color.a * uTransparency;

	color = color * uBlendColor;

	// color = vec4(mod(fTexCoord.xy, 1.0), 0.0, color.a);
	/*
	color.a = 1.0;
	color.r = mod(fTexCoord.y*2.0, 1.0);
	color.b = mod(fTexCoord.x*2.0, 1.0);
	color.g = 0.0;
	*/
	// color.rgb -= 0.0005;
	// color.a = 1.0;
	//color.rgb = fVertexColor.rgb;

	oFragColor = color;
}
