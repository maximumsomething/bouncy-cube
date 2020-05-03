#version 410 core

out vec4 FragColor;


void main() {
	int r = (gl_PrimitiveID & 0x000000FF) >>  0;
	int g = (gl_PrimitiveID & 0x0000FF00) >>  8;
	int b = (gl_PrimitiveID & 0x00FF0000) >> 16;
	
	/*int i = gl_PrimitiveID;
	int r = (((i >> 0) & 1) << 7) | (((i >> 3) & 1) << 6) | (((i >> 6) & 1) << 5)
	| (((i >> 9) & 1) << 4);
	int g = (((i >> 1) & 1) << 7) | (((i >> 4) & 1) << 6) | (((i >> 7) & 1) << 5)
	| (((i >> 10) & 1) << 4);
	int b = (((i >> 2) & 1) << 7) | (((i >> 5) & 1) << 6) | (((i >> 8) & 1) << 5)
	| (((i >> 11) & 1) << 4);*/
	
	FragColor = vec4(r/255.0, g/255.0, b/255.0, 1.0);
}
