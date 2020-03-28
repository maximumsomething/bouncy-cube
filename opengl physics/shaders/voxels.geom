#version 330 core

// Not caring about sides right now, just rendering all faces.
//uniform samplerBuffer vertices;

layout (points) in;
layout (triangle_strip, max_vertices = 24) out;

in int exposedFaces[];

uniform mat4 transform;

vec4 makeCorner(vec3 directions) {
	return gl_in[0].gl_Position + transform * vec4(directions * 0.5, 0.0);
}
void emitFace(vec4[4] corners) {
	for (int i = 0; i < 4; ++i) {
		gl_Position = corners[i];
		EmitVertex();
	}
	EndPrimitive();
}

bool bit(int field, int bit) {
	//return (field | (1 << bit)) != 0;
	return true;
}

void main() {
	
	// m=minus p=plus
	vec4 cornermmm = makeCorner(vec3(-1, -1, -1));
	vec4 cornerpmm = makeCorner(vec3( 1, -1, -1));
	vec4 cornermpm = makeCorner(vec3(-1,  1, -1));
	vec4 cornerppm = makeCorner(vec3( 1,  1, -1));
	vec4 cornermmp = makeCorner(vec3(-1, -1,  1));
	vec4 cornerpmp = makeCorner(vec3( 1, -1,  1));
	vec4 cornermpp = makeCorner(vec3(-1,  1,  1));
	vec4 cornerppp = makeCorner(vec3( 1,  1,  1));
	
	// -x face
	if (bit(exposedFaces[0], 0)) emitFace(vec4[](cornermmm, cornermmp, cornermpm, cornermpp));
	// +x face
	if (bit(exposedFaces[0], 3)) emitFace(vec4[](cornerpmm, cornerpmp, cornerppm, cornerppp));
	// -y face
	if (bit(exposedFaces[0], 1)) emitFace(vec4[](cornermmm, cornermmp, cornerpmm, cornerpmp));
	// +y face
	if (bit(exposedFaces[0], 4)) emitFace(vec4[](cornermpm, cornermpp, cornerppm, cornerppp));
	// -z face
	if (bit(exposedFaces[0], 2)) emitFace(vec4[](cornermmm, cornermpm, cornerpmm, cornerppm));
	// +z face
	if (bit(exposedFaces[0], 5)) emitFace(vec4[](cornermmp, cornermpp, cornerpmp, cornerppp));
}
