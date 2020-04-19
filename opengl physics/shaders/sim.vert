#version 410 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inTurn;
layout (location = 2) in vec3 inVel;
layout (location = 3) in vec3 inAngVel;

layout (location = 4) in ivec3 neighborsM;
layout (location = 5) in ivec3 neighborsP;

out vec3 outPos;
out vec3 outTurn;
out vec3 outVel;
out vec3 outAngVel;
out float debugFeedback;

uniform float timeDelta;

uniform samplerBuffer allVerts;

@include "quaternion.glsl"


vec4 inTurnQuat = quat_from_axisAngle(inTurn);

//const float groundY = 0;
//const float timeDelta = 0.0083;
// in force per distance
const float materialSpringiness = 3000;
const float materialTwistiness = 500;

const float cubeMass = 1;

const float dampingFactor = 0.05;
const float angDampingFactor = 0.02;

//const float gravity = 32;
const float gravity = 0;

const float floorY = -50;


// returns force, given offsets
const float turn = 0.5;
vec3 spring(vec3 offsets) {
	/*if (length(offsets) < turn) return offsets * 3000;
	else {
		vec3 offsetsDir = normalize(offsets);
		return 1000 * turn * offsetsDir + (offsets - offsetsDir * turn) * 5000;
	}*/
	return offsets * materialSpringiness;
}

vec3 projVec(vec3 toProject, vec3 along) {
	along = normalize(along);
	return along * dot(toProject, along);
}

float normAngle(float inAngle) {
	inAngle = mod(inAngle + PI, PI * 2);
	if (inAngle < 0) inAngle += PI * 2;
	return inAngle - PI;
}
vec3 normAxisAngle(vec3 inAxisAngle) {
	float inAngle = length(inAxisAngle);
	if (abs(inAngle) < PI) return inAxisAngle;
	return inAxisAngle / inAngle * normAngle(inAngle);
}

vec3 offsets = vec3(0, 0, 0);
vec3 angOffsets = vec3(0, 0, 0);
vec3 twists = vec3(0, 0, 0);
vec3 neighVels = vec3(0, 0, 0);
vec3 neighAngVels = vec3(0, 0, 0);
float neighborAmount = 0;

void checkNeighbor(int neighborIdx, vec3 baseNormal) {
	if (neighborIdx == -1) return;
	vec3 normal = quat_rotate_vector(baseNormal / 2, inTurnQuat);
	vec3 neighTurn = texelFetch(allVerts, neighborIdx * 4 + 1).xyz;
	vec3 neighborNormal = quat_rotate_vector(-baseNormal / 2, quat_from_axisAngle(neighTurn));
	vec3 neighPos = texelFetch(allVerts, neighborIdx * 4).xyz;
	vec3 offset = (neighPos + neighborNormal) - (inPos + normal);
	offsets += offset;
	vec3 angOffset = cross(normal, offset);
	angOffsets += angOffset;
	vec3 twistOffset = normAxisAngle(projVec(neighTurn, normal) - projVec(inTurn, normal));
	twists += twistOffset;
	debugFeedback += length(offset) + length(angOffset) + length(twistOffset);
	
	
	++neighborAmount;
	neighVels += texelFetch(allVerts, neighborIdx * 4 + 2).xyz;
	neighAngVels += texelFetch(allVerts, neighborIdx * 4 + 3).xyz;
	
}

void main() {
	debugFeedback = 0;
	checkNeighbor(neighborsM.x, vec3(-1, 0, 0));
	checkNeighbor(neighborsM.y, vec3(0, -1, 0));
	checkNeighbor(neighborsM.z, vec3(0, 0, -1));
	checkNeighbor(neighborsP.x, vec3(1, 0, 0));
	checkNeighbor(neighborsP.y, vec3(0, 1, 0));
	checkNeighbor(neighborsP.z, vec3(0, 0, 1));
	
	if (neighborAmount > 0) {
		neighVels /= neighborAmount;
		neighAngVels /= neighborAmount;
	}
	outVel = mix(inVel, neighVels, dampingFactor * neighborAmount);
	outVel = outVel + (spring(offsets) / cubeMass + vec3(0, -gravity, 0)) * timeDelta;
	
	vec3 dampedInAngVel = mix(inAngVel, neighAngVels, angDampingFactor * neighborAmount);
	//outAngVel = quat_to_axisAngle(quat_mul(quat_from_axisAngle(dampedInAngVel), quat_from_axisAngle((spring(angOffsets) + materialTwistiness * twists) / cubeMass * timeDelta)));
	outAngVel = dampedInAngVel + (spring(angOffsets) + materialTwistiness * twists) / cubeMass * timeDelta;
	//outAngVel = vec3(0, 0, 0);
	
	// basic ass collision checking
	if (inPos.y < floorY) outVel.y = max(outVel.y, 0);
	
	outPos = inPos + outVel * timeDelta;
	//outTurn = quat_mul(quat_from_axisAngle(outAngVel * timeDelta), inTurn);
	//outTurn = vec3(0, 0, 0);
	//outTurn = inTurn + outAngVel * timeDelta;
	outTurn = normAxisAngle(quat_to_axisAngle(quat_mul(quat_from_axisAngle(outAngVel * timeDelta), inTurnQuat)));
}
