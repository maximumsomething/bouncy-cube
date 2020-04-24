#version 410 core

@include "quaternion.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inTurn;
layout (location = 2) in vec3 inVel;
layout (location = 3) in vec3 inAngVel;

layout (location = 4) in ivec3 neighborsM;
layout (location = 5) in ivec3 neighborsP;

out vec3 outPos;
out vec4 outTurn;
out vec3 outVel;
out vec3 outAngVel;
out float debugFeedback;

uniform float timeDelta;

uniform samplerBuffer allVerts3D;
uniform samplerBuffer allVerts4D;


//const float groundY = 0;
//const float timeDelta = 0.0083;
// in force per distance
const float materialSpringiness = 3000;
const float materialTwistiness = 1000;

const float cubeMass = 1;

const float dampingFactor = 0.05;
const float angDampingFactor = 0.05;

const float gravity = 32;
//const float gravity = 0;

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
	if (inAngle < PI) return inAxisAngle;
	return inAxisAngle / inAngle * normAngle(inAngle);
}

vec3 offsets = vec3(0, 0, 0);
vec3 angOffsets = vec3(0, 0, 0);
vec3 twists = vec3(0, 0, 0);
vec3 neighVels = vec3(0, 0, 0);
vec3 neighAngVels = vec3(0, 0, 0);
float neighborAmount = 0;

// Torques, angular velocities, etc. can be simply added together as axisAngle vectors. This is because the rotation is infinitesimal. If it's a rotation in actual space, however, it must be applied in a more complicated way, which is done here by converting them into quaternions.

void checkNeighbor(int neighborIdx, vec3 baseNormal) {
	if (neighborIdx == -1) return;
	vec3 normal = quat_rotate_vector(baseNormal / 2, inTurn);
	//vec3 normal = baseNormal / 2;
	
	vec3 neighPos = texelFetch(allVerts3D, neighborIdx * 3).xyz;
	vec4 neighTurn = texelFetch(allVerts4D, neighborIdx);
	vec3 neighVel = texelFetch(allVerts3D, neighborIdx * 3 + 1).xyz;
	vec3 neighAngVel = texelFetch(allVerts3D, neighborIdx * 3 + 2).xyz;
	
	vec3 neighborNormal = quat_rotate_vector(-baseNormal / 2, neighTurn);
	//vec3 neighborNormal = -baseNormal / 2;
	
	vec3 offset = (neighPos + neighborNormal) - (inPos + normal);
	offsets += offset;
	vec3 angOffset = cross(normal, offset);
	angOffsets += angOffset;
	//vec3 twistOffset = normAxisAngle(projVec(neighTurn, normal) - projVec(inTurn, normal));
	//vec3 twistOffset = normAxisAngle(neighTurn - inTurn);
	// Once it becomes an "offset", it is no longer simply a rotation, but a torque divided by a value of (torque per angular offset).
	vec3 twistOffset = normAxisAngle(quat_to_axisAngle(quat_mul(neighTurn, quat_conj(inTurn))));
	twists += twistOffset;
	debugFeedback += length(offset) + length(angOffset) + length(twistOffset);
	
	
	++neighborAmount;
	
	neighVels += neighVel + cross(neighAngVel, neighborNormal) - cross(inAngVel, normal);
	neighAngVels += neighAngVel;
	
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
	
	// slightly less basic ass collision checking
	if (inPos.y < floorY) offsets.y += (floorY - inPos.y);
	
	outVel = mix(inVel, neighVels, dampingFactor * neighborAmount);
	outVel = outVel + (spring(offsets) / cubeMass + vec3(0, -gravity, 0)) * timeDelta;
	
	vec3 dampedInAngVel = mix(inAngVel, neighAngVels, angDampingFactor * neighborAmount);
	outAngVel = dampedInAngVel + (spring(angOffsets) + materialTwistiness * twists) / cubeMass * timeDelta;
	//outAngVel = vec3(0, 0, 0);
	
	// basic ass collision checking
	//if (inPos.y < floorY) outVel.y = max(outVel.y, 0);
	
	outPos = inPos + outVel * timeDelta;
	//outTurn = vec3(0, 0, 0);
	//outTurn = inTurn + outAngVel * timeDelta;
	outTurn = quat_mul(quat_from_axisAngle(outAngVel * timeDelta), inTurn);
	//outTurn = QUATERNION_IDENTITY;
}
