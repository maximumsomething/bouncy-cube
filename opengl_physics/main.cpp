#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <execinfo.h>

#include "bridge.hpp"
#include "loaders.hpp"
#include "input.hpp"
#include "voxels.hpp"


#ifndef __APPLE__
#define DEBUG_OUTPUT_SUPPORTED
#endif


class Skybox {
	
	GLuint skyboxTexture = loadCubemap({
		"skybox/right.jpg",
		"skybox/left.jpg",
		"skybox/top.jpg",
		"skybox/bottom.jpg",
		"skybox/front.jpg",
		"skybox/back.jpg"
	});
	GLuint shaderProgram = loadShaders("skybox.vert", "skybox.frag");
	
	GLuint VBO;
	GLuint VAO;
	
public:
	Skybox() {
		float vertices[] = {
			// positions
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};
		
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (float*)0);
		glEnableVertexAttribArray(0);
	}
	void render(glm::mat4 view, glm::mat4 projection) {
		glDepthMask(GL_FALSE);
		glUseProgram(shaderProgram);
		
		view = glm::mat4(glm::mat3(view));
		
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		
		glBindVertexArray(VAO);
		
		//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
		
		glDrawArrays(GL_TRIANGLES, 0, 36);
		
		glDepthMask(GL_TRUE);
	}
};

class HelloTriangle {
	
	Skybox skybox;
	GLuint shaderProgram = loadShaders("intro.vert", "intro.frag");
	GLuint VBO;
	GLuint VAO;
	GLuint floorTexture;
	
	glm::vec3 cameraUp = glm::vec3(0, 1, 0);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 cameraDirection = glm::normalize(-cameraPos);
	
	std::unique_ptr<VoxelRenderer> voxelRenderer = getVoxelRenderer();
	
public:
	HelloTriangle() {
		
		/*float vertices[] = {
			// positions         // colors
			 0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
			-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
			 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top
		};
		
		
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (float*)0);
		glEnableVertexAttribArray(0);
		// color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (float*)(3* sizeof(float)));
		glEnableVertexAttribArray(1);
		
		floorTexture = loadTexture("wall.jpg");*/
	}
	
	std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
	
	void processInput(GLFWwindow* window) {
		auto currentFrameTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
		
		float moveAmount = deltaTime*5;
		
		if (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
			moveAmount *= 5;
		
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += moveAmount * glm::normalize(glm::vec3(cameraDirection.x, 0, cameraDirection.z));
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= moveAmount * glm::normalize(glm::vec3(cameraDirection.x, 0, cameraDirection.z));
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraDirection, cameraUp)) * moveAmount;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraDirection, cameraUp)) * moveAmount;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			cameraPos += cameraUp * moveAmount;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			cameraPos -= cameraUp * moveAmount;
		
		
		float dyaw = 0, dpitch = 0;
		
		//std::cout << "cameraDirection: x:" << cameraDirection.x << " y:" << cameraDirection.y << " z:" << cameraDirection.z <<
		//" yaw:" << dyaw << " pitch:" << dpitch << std::endl;
		
		float turnAmount = deltaTime * 2;
		
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			dpitch += turnAmount;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			dpitch -= turnAmount;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			dyaw -= turnAmount;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			dyaw += turnAmount;
		
		float oldPitch = asin(cameraDirection.y);
		
		cameraDirection = glm::rotate(cameraDirection, dyaw, -cameraUp);
		// Prevent the camera from flipping backwards
		if (abs(oldPitch + dpitch) <= M_PI_2 - 0.01) cameraDirection = glm::rotate(cameraDirection, dpitch, glm::cross(cameraDirection, cameraUp));
		
		
		lastFrameTime = currentFrameTime;
	}
	
	void render() {
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		glm::mat4 view = glm::lookAt(cameraPos,
									 cameraPos + cameraDirection,
									 cameraUp);
		
		
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(00.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 projection;
		projection = glm::perspective(glm::radians(60.0f), (float) windowData.width / windowData.height, 0.1f, 10000.0f);
		
		
		skybox.render(view, projection);
		voxelRenderer->render(view, projection);
		
		/*
		glUseProgram(shaderProgram);
		
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);*/
	}
};


#ifdef DEBUG_OUTPUT_SUPPORTED
// Straight from https://learnopengl.com/In-Practice/Debugging
void APIENTRY myDebugOutput(GLenum source,
							GLenum type,
							GLuint id,
							GLenum severity,
							GLsizei length,
							const GLchar *message,
							const void *userParam)
{
	// ignore non-significant error/warning codes
	if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " <<  message << std::endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;

	/*std::cout << "Backtrace:" << std::endl;
	// https://stackoverflow.com/a/77336/4062079
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	backtrace_symbols_fd(array, size, STDOUT_FILENO);*/
	std::cout << std::endl;
}
#endif

int main(int argc, char** argv) {
	srand(time(0));
	
	glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef DEBUG_OUTPUT_SUPPORTED
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(800, 600, "Boingboing", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	windowData.window = window;
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	
	GLint flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	
#ifdef DEBUG_OUTPUT_SUPPORTED
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(myDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif
	
	glViewport(0, 0, windowData.width, windowData.height);
	
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* windoww, int width, int height) {
		windowData.width = width; windowData.height = height;
		glViewport(0, 0, width, height);
	});
		
	setupInput(window);
	
	addKeyListener(GLFW_KEY_ESCAPE, [window](int scancode, int action, int mods) {
		 glfwSetWindowShouldClose(window, true);
	});
	
	
	HelloTriangle renderer;
	
	
	std::cout << glGetString(GL_VERSION) << std::endl;
	
	// main loop
	while(!glfwWindowShouldClose(window))
	{
		renderer.processInput(window);
		renderer.render();
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	return 0;
}
