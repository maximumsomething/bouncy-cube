#version 410 core
// Very simple; just creates the texture coord.

layout (lines_adjacency) in;
layout (triangle_strip, max_vertices = 4) out;

in float feedback[];

out vec2 texCoord;
// PREMULTIPLIED
out vec4 vertColor;

uniform samplerBuffer faceHighlight;

float highlight;

void doVertex(int i, vec2 coord) {
	texCoord = coord;
	float dfHighlight = min(feedback[i] * 0.25, 0.5);
	float meanHighlight = (dfHighlight + highlight) / 2;
	vertColor = vec4(dfHighlight, highlight, 0, meanHighlight);
	gl_Position = gl_in[i].gl_Position;
	EmitVertex();
}

void main() {
	highlight = texelFetch(faceHighlight, gl_PrimitiveIDIn).r * 0.5;
	doVertex(0, vec2(0, 0));
	doVertex(1, vec2(1, 0));
	doVertex(3, vec2(0, 1));
	doVertex(2, vec2(1, 1));
}
