#version 410 core
// Very simple; just creates the texture coord.

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

in float feedback[];

out vec2 texCoord;
out vec4 vertColor;

void doVertex(int i, vec2 coord) {
	texCoord = coord;
	vertColor = vec4(1, 0, 0, min(feedback[i] * 0.25, 0.5));
	gl_Position = gl_in[i].gl_Position;
	EmitVertex();
}

void main() {
	
	doVertex(0, vec2(0, 0));
	doVertex(1, vec2(1, 0));
	doVertex(3, vec2(0, 1));
	doVertex(2, vec2(1, 1));
}
