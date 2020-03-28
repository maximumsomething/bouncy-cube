#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in int neighbors[6];

uniform mat4 transform;

// Stored in least significant bits
out int exposedFaces = 0;

void main() {
	gl_Position = transform * vec4(aPos, 1.0);
	//for (int i = 0; i < 6; ++i) if (neighbors[i] == -1) exposedFaces |= 1 << i;
	exposedFaces = 3;
}
