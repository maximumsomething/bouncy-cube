
#version 410 core
// Simpler version of stretchyVoxels.geom that doesn't care about anything other than position

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;


void main() {
	gl_PrimitiveID = gl_PrimitiveIDIn;
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = gl_in[3].gl_Position;
	EmitVertex();
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();
}
