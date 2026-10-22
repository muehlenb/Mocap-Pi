#version 330

out vec4 fragment_color;

in vec4 world_position;

uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

void main()
{
    fragment_color = color;
}
