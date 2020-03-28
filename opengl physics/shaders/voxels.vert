#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 neighborsM;
layout (location = 2) in ivec3 neighborsP;

//out vec3 cubeColor;

uniform mat4 transform;

// Stored in least significant bits
out int exposedFaces = 0;

void main() {
	gl_Position = transform * vec4(aPos, 1.0);
	
	//int neighbors[6] = int[6](neighborsP.xyz, neighborsM.xyz);
	//for (int i = 0; i < 6; ++i) if (neighbors[i] == -1) exposedFaces |= (1 << i);
	if (neighborsM.x == -1) exposedFaces |= (1 << 0);
	if (neighborsM.y == -1) exposedFaces |= (1 << 1);
	if (neighborsM.z == -1) exposedFaces |= (1 << 2);
	if (neighborsP.x == -1) exposedFaces |= (1 << 3);
	if (neighborsP.y == -1) exposedFaces |= (1 << 4);
	if (neighborsP.z == -1) exposedFaces |= (1 << 5);
	
	//cubeColor = vec3(float(gl_VertexID), .5, 0.5);
	//cubeColor = vec3(.5, .5, .5);
}
