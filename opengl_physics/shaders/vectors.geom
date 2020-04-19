
#version 410 core

layout (points) in;
layout (line_strip, max_vertices = 6) out;

uniform mat4 transform;


in PHYS_PROPS {
	vec3 turn;
	vec3 vel;
	vec3 angVel;
} vertIn[];

out vec4 lineColor;

void drawLine(vec3 color, vec3 pointing) {
	lineColor = vec4(color, 1);
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = gl_in[0].gl_Position + transform * vec4(pointing, 0);
	EmitVertex();
	EndPrimitive();
}

// Velocity: green, Angular velocity: purple, Current angle: Blue
void main() {
	drawLine(vec3(0, 1, 0), vertIn[0].vel * 0.3);
	drawLine(vec3(1, 0, 1), vertIn[0].angVel * 0.3);
	drawLine(vec3(0, 0, 1), vertIn[0].turn);
}
