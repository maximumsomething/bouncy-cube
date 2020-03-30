#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inVel;
layout (location = 2) in ivec3 neighborsM;
layout (location = 3) in ivec3 neighborsP;

out vec3 outPos;
out vec3 outVel;

uniform samplerBuffer allVertsPos;
uniform samplerBuffer allVertsVel;

//const float groundY = 0;
const float timeDelta = 0.0083;
// in force per distance
const float materialSpringiness = 3000;
const float cubeMass = 1;

const float dampingFactor = 0.05;

const float gravity = 32;

const float floorY = -50;


// returns force
const float turn = 0.5;
vec3 spring(vec3 offsets) {
	if (length(offsets) < turn) return offsets * 3000;
	else {
		vec3 offsetsDir = normalize(offsets);
		return 1000 * turn * offsetsDir + (offsets - offsetsDir * turn) * 5000;
	}
}


vec3 offsets = vec3(0, 0, 0);
vec3 neighVels = vec3(0, 0, 0);
float neighborAmount = 0;

void checkNeighbor(int neighborIdx, vec3 normal) {
	if (neighborIdx == -1) return;
	vec3 neighPos = texelFetch(allVertsPos, neighborIdx).xyz;
	offsets += (neighPos - (inPos + normal));
	
	++neighborAmount;
	neighVels += texelFetch(allVertsVel, neighborIdx).xyz;
	
}


void main() {
	checkNeighbor(neighborsM.x, vec3(-1, 0, 0));
	checkNeighbor(neighborsM.y, vec3(0, -1, 0));
	checkNeighbor(neighborsM.z, vec3(0, 0, -1));
	checkNeighbor(neighborsP.x, vec3(1, 0, 0));
	checkNeighbor(neighborsP.y, vec3(0, 1, 0));
	checkNeighbor(neighborsP.z, vec3(0, 0, 1));
	
	if (neighborAmount > 0) neighVels /= neighborAmount;
	outVel = mix(inVel, neighVels, dampingFactor * neighborAmount);
	outVel = outVel + (spring(offsets) / cubeMass + vec3(0, -gravity, 0)) * timeDelta;
	
	
	// basic ass collision checking
	if (inPos.y < floorY) outVel.y = max(outVel.y, 0);
	outPos = inPos + outVel * timeDelta;
}
