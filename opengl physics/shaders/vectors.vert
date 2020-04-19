
#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 turn;
layout (location = 2) in vec3 vel;
layout (location = 3) in vec3 angVel;


uniform mat4 transform;


out PHYS_PROPS {
	vec3 turn;
	vec3 vel;
	vec3 angVel;
} vertOut;

void main() {
	vertOut.turn = turn;
	vertOut.vel = vel;
	vertOut.angVel = angVel;
	
	gl_Position = transform * vec4(aPos, 1);
}
