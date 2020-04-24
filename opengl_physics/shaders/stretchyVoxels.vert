#version 410 core


@include "quaternion.glsl"

layout (location = 0) in ivec4 neighborsM;
layout (location = 1) in ivec4 neighborsP;

uniform mat4 transform;

uniform samplerBuffer allVerts3D;
uniform samplerBuffer allVerts4D;


vec3 totalPositions = vec3(0, 0, 0);
float numNeighbors = 0;

vec3 beep;

void getNeighborCorner(int idx, vec3 pointing) {
	if (idx == -1) return;
	vec3 pos = texelFetch(allVerts3D, idx * 3).xyz;
	vec4 turn = texelFetch(allVerts4D, idx);
	totalPositions += pos + /*pointing * 0.5*/quat_rotate_vector(pointing * 0.5, turn);
	++numNeighbors;
}

void main() {
	getNeighborCorner(neighborsM.x, vec3( 1,  1,  1));
	getNeighborCorner(neighborsM.y, vec3(-1,  1,  1));
	getNeighborCorner(neighborsM.z, vec3( 1, -1,  1));
	getNeighborCorner(neighborsM.w, vec3(-1, -1,  1));
	getNeighborCorner(neighborsP.x, vec3( 1,  1, -1));
	getNeighborCorner(neighborsP.y, vec3(-1,  1, -1));
	getNeighborCorner(neighborsP.z, vec3( 1, -1, -1));
	getNeighborCorner(neighborsP.w, vec3(-1, -1, -1));
	
	gl_Position = transform * vec4(totalPositions / numNeighbors, 1.0);
}

