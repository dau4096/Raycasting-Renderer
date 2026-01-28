/* generic.vert */
#version 460 core

out vec2 fragUV;

const vec2 positions[4] = vec2[4](
	vec2(-1.0, -1.0), //Bottom Left
	vec2( 1.0, -1.0), //Bottom Right
	vec2(-1.0,  1.0), //Top Left
	vec2( 1.0,  1.0)  //Top Right
);

const vec2 uv[4] = vec2[4](
	vec2(0.0, 0.0), //Bottom Left
	vec2(1.0, 0.0), //Bottom Right
	vec2(0.0, 1.0), //Top Left
	vec2(1.0, 1.0)  //Top Right
);

void main() {
	gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
	fragUV = uv[gl_VertexID].xy;
}