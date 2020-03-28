#version 330 core
out vec4 FragColor;
//in vec3 vertColor;

in vec2 texCoord;
uniform sampler2D cubeTexture;

void main() {
	//FragColor = vec4(.5, .5, .5, .5);
	//FragColor = vec4(vertColor, 1);
	
	FragColor = vec4(texture(cubeTexture, texCoord).rgb, 1);
}
