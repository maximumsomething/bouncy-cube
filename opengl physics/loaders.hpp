#pragma once

#include <string>
#include <vector>


unsigned int loadTexture(const char* name);
unsigned int loadCubemap(const std::vector<std::string>& faces);


unsigned int loadShaders(const char* vert, const char* frag);


