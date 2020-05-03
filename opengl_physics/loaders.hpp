#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glad/glad.h>


GLuint loadTexture(const char* name);
GLuint loadCubemap(const std::vector<std::string>& faces);

GLuint loadShader(const char* name, GLenum type);
GLuint linkShaders(std::vector<GLuint> shaders, bool deleteShaders = true, std::function<void(GLuint)> preLinkOptions = nullptr);

GLuint loadShaders(const char* vert, const char* frag);

void dumpImage(uint8_t* data, int width, int height);

GLenum glCheckError_(const char *file, int line);
#define glCheckError() glCheckError_(__FILE__, __LINE__) 
