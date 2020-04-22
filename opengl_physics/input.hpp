#ifndef INPUT_HPP
#define INPUT_HPP

#include <functional>
#include <GLFW/glfw3.h>

struct WindowData {
	GLFWwindow* window;
	int width = 800;
	int height = 600;
};
extern WindowData windowData;



// int scancode, int action, int mods
void addKeyListener(int key, std::function<void(int, int, int)> callback);
// int button, int action, int mods
void addClickListener(std::function<void(int, int, int)> callback);


void setupInput(GLFWwindow* window);



#endif // INPUT_HPP
