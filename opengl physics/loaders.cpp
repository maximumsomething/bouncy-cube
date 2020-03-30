#include "loaders.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "bridge.hpp"

// Must have already bound texture
bool loadGLImage(const char* name, GLenum target) {
	int width, height, nrChannels;
	unsigned char *data = stbi_load((getResourcesPath() + "/textures/" + name).c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	
	else {
		std::cout << "Failed to load texture " << name << std::endl;
	}
	stbi_image_free(data);
	
	return (bool) data;
}

GLuint loadTexture(const char* name) {
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	loadGLImage(name, GL_TEXTURE_2D);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	return texture;
}

GLuint loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (unsigned int i = 0; i < faces.size(); i++) {
		loadGLImage(faces[i].c_str(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint loadShader(const char* name, GLenum type) {
	
	FILE* file = fopen((getResourcesPath() + "/shaders/" + name).c_str(), "r");
	if (file == nullptr) {
		perror((std::string("shader ") + name).c_str());
		glfwTerminate();
		exit(1);
	}
	
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);  /* same as rewind(f); */

	char *shaderString = (char *) malloc(fsize + 1);
	fread(shaderString, 1, fsize, file);
	fclose(file);
	shaderString[fsize] = 0;
	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderString, NULL);
	glCompileShader(shader);
	
	int  success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	
	if(!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "ERROR: SHADER " << name << " COMPILATION FAILED\n" << infoLog << std::endl;
		glfwTerminate();
		exit(1);
	}
	free(shaderString);
	return shader;
}

GLuint linkShaders(std::vector<GLuint> shaders, std::function<void(GLuint)> preLinkOptions) {
	
	GLuint shaderProgram = glCreateProgram();
	for (GLuint i : shaders) {
		glAttachShader(shaderProgram, i);
	}
	if (preLinkOptions != nullptr) preLinkOptions(shaderProgram);
	
	glLinkProgram(shaderProgram);
	
	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR: SHADER LINKING FAILED\n" << infoLog << std::endl;
		glfwTerminate();
		exit(1);
	}
	
	for (GLuint i : shaders) {
		glDeleteShader(i);
	}
	
	return shaderProgram;
}

GLuint loadShaders(const char* vert, const char* frag) {
	
	GLuint vertexShader = loadShader(vert, GL_VERTEX_SHADER);
	GLuint fragmentShader = loadShader(frag, GL_FRAGMENT_SHADER);
	
	return linkShaders({vertexShader, fragmentShader});
}

GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}

