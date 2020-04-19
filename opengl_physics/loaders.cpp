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

std::string loadShaderFile(const char* name) {
	FILE* file = fopen((getResourcesPath() + "/shaders/" + name).c_str(), "r");
	if (file == nullptr) {
		perror((std::string("Error loading shader ") + name).c_str());
		glfwTerminate();
		exit(1);
	}
	
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);  /* same as rewind(f); */

	std::string shaderString(fsize, '\0');
	fread(&shaderString.front(), 1, fsize, file);
	fclose(file);
	return shaderString;
}

// Processes includes. Must be of the form @include "file" as the first thing on a line
GLuint loadShader(const char* name, GLenum type) {
	
	std::string shaderString = loadShaderFile(name);
	
	// Very quick and dirty include handling
	std::vector<std::string> includeFiles;
	std::vector<const char*> stringPtrs(1, shaderString.c_str());
	
	size_t lastPos = 0;
	while ((lastPos = shaderString.find("\n@include", lastPos)) != std::string::npos) {
		
		size_t lineEnd = shaderString.find("\n", lastPos + 1);
		std::string line = shaderString.substr(lastPos, lineEnd - lastPos);
		size_t quotedStart = line.find("\"") + 1;
		std::string quoted = line.substr(quotedStart, line.rfind("\"") - quotedStart);
		
		if (quoted.length() < 1) {
			std::cout << "Shader include error: not enough quotes, or nothing between them" << std::endl;
		}
		
		includeFiles.push_back(loadShaderFile(quoted.c_str()));
		stringPtrs.push_back(includeFiles.back().c_str());
		shaderString[lastPos] = '\0';
		stringPtrs.push_back(&shaderString[lineEnd]);
	}
	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, stringPtrs.size(), stringPtrs.data(), NULL);
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

