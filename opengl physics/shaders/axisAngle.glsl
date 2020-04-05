
#define PI 3.1415926

//https://gist.github.com/yiwenl/3f804e80d0930e34a0b33359259b556c
// by is an axis/angle vector: toRotate is rotated by length(by) radians around the vector by
mat3 rotMatFromAxisAngle(vec3 axisAngle) {
	float angle = length(axisAngle);
	if (angle == 0) return mat3(1.0);
	vec3 axis = axisAngle / angle;
	float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
				oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
				oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
}

float normAngle(float x) {
	x = mod(x + PI,PI*2);
	if (x < 0) x += PI*2;
	return x - PI;
}
