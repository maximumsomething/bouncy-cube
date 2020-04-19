#version 410 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{

	//FragColor = mix(vec4((TexCoords + 1.0) / 2.0, 1.0), texture(skybox, TexCoords), 0.5);
	FragColor = texture(skybox, TexCoords);
}
