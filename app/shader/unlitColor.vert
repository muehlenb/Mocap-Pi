#version 330

layout(location = 0) in vec4 inPosition;

/** World position */
out vec4 world_position;

/**
 * Projection matrix which performs a perspective transformation.
 */
uniform mat4 projection;

/**
 * View matrix which transforms the vertices from world space
 * into camera space.
 */
uniform mat4 view;

/**
 * Model matrix which transforms the vertices from model space
 * into world space.
 */
uniform mat4 model;

/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{	
	world_position = model * inPosition;

	gl_PointSize = 5;

    // Calculate gl_Position:
    gl_Position = projection * view * world_position;
}
