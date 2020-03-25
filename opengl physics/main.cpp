#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>

#include "bridge.hpp"
#include "loaders.hpp"


struct {
	GLFWwindow* window;
	int width = 800;
	int height = 600;
} windowData;


void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}



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
	}
};

class HelloTriangle {
	
	Skybox skybox;
	GLuint shaderProgram = loadShaders("intro.vert", "intro.frag");
	GLuint VBO;
	GLuint VAO;
	GLuint floorTexture;
	
	glm::vec3 cameraUp = glm::vec3(0, 1, 0);
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraDirection = -cameraPos;
	
public:
	HelloTriangle() {
		
		float vertices[] = {
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
		
		floorTexture = loadTexture("wall.jpg");
		
	}
	
	std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
	
	void processInput(GLFWwindow* window) {
		auto currentFrameTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
		
		float moveAmount = deltaTime*2;
		
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += moveAmount * cameraDirection;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= moveAmount * cameraDirection;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraDirection, cameraUp)) * moveAmount;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraDirection, cameraUp)) * moveAmount;
		
		float yaw = atan2(cameraDirection.z, cameraDirection.x);
		float pitch = asin(cameraDirection.y);
		
		
		//std::cout << "cameraDirection: x:" << cameraDirection.x << " y:" << cameraDirection.y //<< " z:" << cameraDirection.z <<
		//" yaw:" << yaw << " pitch:" << pitch << std::endl;
		
		float turnAmount = deltaTime * 2;
		
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			pitch += turnAmount;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			pitch -= turnAmount;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			yaw -= turnAmount;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			yaw += turnAmount;
		
		cameraDirection = glm::vec3(cos(yaw), sin(pitch), sin(yaw));
		
		lastFrameTime = currentFrameTime;
	}
	
	void render() {
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		
		glm::mat4 view = glm::lookAt(cameraPos,
									 cameraPos + cameraDirection,
									 cameraUp);
		
		
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, glm::radians(00.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 projection;
		projection = glm::perspective(glm::radians(60.0f), (float) windowData.width / windowData.height, 0.1f, 100.0f);
		
		
		skybox.render(view, projection);
		
		
		glUseProgram(shaderProgram);
		
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
};


int main(int argc, char** argv) {
	
	glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	
	GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
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

	
	glViewport(0, 0, windowData.width, windowData.height);
	
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		windowData.width = width; windowData.height = height;
		glViewport(0, 0, width, height);
	});
	
	
	HelloTriangle renderer;
	
	// main loop
	while(!glfwWindowShouldClose(window))
	{
		processInput(window);
		
		renderer.processInput(window);
		renderer.render();
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	glfwTerminate();
	return 0;
}
