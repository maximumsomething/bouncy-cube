#ifndef INPUT_HPP
#define INPUT_HPP

#include <functional>
#include <GLFW/glfw3.h>

void addKeyListener(int key, std::function<void(int, int, int)> callback);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


#endif // INPUT_HPP
