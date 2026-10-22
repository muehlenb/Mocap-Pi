#version 330 core
layout (location = 0) in vec2 aPos; // the position variable
layout (location = 1) in vec2 aTexCoord; // the texcoord variable

uniform int x;
uniform int y;

uniform float fontSize;

uniform float screenWidth;
uniform float screenHeight;

out vec2 uv;

void main()
{
	uv = aTexCoord;

	// Vertex + Offset (in pixels):
	vec2 pos = aPos * fontSize + vec2(x,y);
	
	float aspect = screenWidth / screenHeight;
	
	vec2 normalizedPos = pos * 2 / vec2(screenWidth, screenHeight) - vec2(1, 1);
	
    gl_Position = vec4(normalizedPos, 0, 1);
}