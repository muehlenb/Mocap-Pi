#version 330 core

uniform sampler2D fontTexture;

in vec2 uv;

out vec4 FragColor;

void main()
{
	float value = texture(fontTexture, uv).r;

	if(value < 0.01)
		discard;

	FragColor = vec4(1,1,1,value);
}
