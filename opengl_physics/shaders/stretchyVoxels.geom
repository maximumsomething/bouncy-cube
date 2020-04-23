#version 410 core
// Very simple; just creates the texture coord.

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

out vec2 texCoord;
out vec4 vertColor;

void main() {
	vertColor = vec4(0, 0, 0, 0);
	
	texCoord = vec2(0, 0);
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	
	texCoord = vec2(1, 0);
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();
	
	texCoord = vec2(0, 1);
	gl_Position = gl_in[3].gl_Position;
	EmitVertex();
	
	texCoord = vec2(1, 1);
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();
}
