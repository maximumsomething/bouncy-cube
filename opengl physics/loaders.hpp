#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>


GLuint loadTexture(const char* name);
GLuint loadCubemap(const std::vector<std::string>& faces);

GLuint loadShader(const char* name, GLenum type);
GLuint linkShaders(std::vector<GLuint> shaders);
GLuint loadShaders(const char* vert, const char* frag);


