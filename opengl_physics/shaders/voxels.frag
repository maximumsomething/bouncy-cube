#version 410 core
out vec4 FragColor;
in vec4 vertColor;

in vec2 texCoord;
uniform sampler2D cubeTexture;

/*
float near = 0.1;
float far  = 10000.0;
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}*/

void main() {
	//FragColor = vec4(.5, .5, .5, .5);
	//FragColor = vec4(vertColor, 1);
	
	//FragColor = vec4(texture(cubeTexture, texCoord).rgb, 1);
	//FragColor = vec4(mix(texture(cubeTexture, texCoord).rgb, vertColor.rgb, vertColor.a), 1);
	FragColor = vec4(texture(cubeTexture, texCoord).rgb * (1 - vertColor.a) + vertColor.rgb, 1);
	//FragColor = vec4(vec3(linearizeDepth(gl_FragCoord.z) / 100), 1.0);
}
