// from https://github.com/mattatz/ShibuyaCrowd/blob/master/source/shaders/common/quaternion.glsl

#define QUATERNION_IDENTITY vec4(0, 0, 0, 1)

#ifndef PI
#define PI 3.1415926
#endif


// Quaternion multiplication
vec4 quat_mul(vec4 q1, vec4 q2) {
	return vec4(
		q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
		q1.w * q2.w - dot(q1.xyz, q2.xyz)
	);
}

// Vector rotation with a quaternion
vec3 quat_rotate_vector(vec3 v, vec4 r) {
	vec4 r_c = r * vec4(-1, -1, -1, 1);
	return quat_mul(r, quat_mul(vec4(v, 0), r_c)).xyz;
}

vec3 quat_rotate_vector_at(vec3 v, vec3 center, vec4 r) {
	vec3 dir = v - center;
	return center + quat_rotate_vector(dir, r);
}

// A given angle of rotation about a given axis
vec4 quat_from_angle_axis(float angle, vec3 axis) {
	float sn = sin(angle * 0.5);
	float cs = cos(angle * 0.5);
	return vec4(axis * sn, cs);
}

vec4 quat_from_axisAngle(vec3 angleAxis) {
	float angle = length(angleAxis);
	if (angle == 0) return QUATERNION_IDENTITY;
	return quat_from_angle_axis(angle, angleAxis / angle);
}

vec3 quat_to_axisAngle(vec4 quat) {
	if (/*quat.xyz == vec3(0, 0, 0) || */quat.w >= 1) return vec3(0, 0, 0);
	return normalize(quat.xyz) * 2*acos(quat.w);
}

vec4 quat_conj(vec4 q) {
	return vec4(-q.x, -q.y, -q.z, q.w);
}
