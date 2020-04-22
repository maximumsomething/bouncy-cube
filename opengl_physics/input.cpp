#include "input.hpp"
#include <map>
#include <vector>


WindowData windowData;

std::map<int, std::function<void(int, int, int)>> keyListeners;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (keyListeners.count(key) != 0) keyListeners[key](scancode, action, mods);
}
// scancode, action, mods
void addKeyListener(int key, std::function<void(int, int, int)> callback) {
	keyListeners[key] = callback;
}

std::vector<std::function<void(int, int, int)>> clickListeners;

void addClickListener(std::function<void(int, int, int)> callback) {
	clickListeners.push_back(callback);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	for (auto i : clickListeners) {
		i(button, action, mods);
	}
}

void setupInput(GLFWwindow* window) {
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}
