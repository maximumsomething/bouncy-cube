#include "input.hpp"
#include <map>


std::map<int, std::function<void(int, int, int)>> keyListeners;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (keyListeners.count(key) != 0) keyListeners[key](scancode, action, mods);
}
// scancode, action, mods
void addKeyListener(int key, std::function<void(int, int, int)> callback) {
	keyListeners[key] = callback;
}
