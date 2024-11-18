#include "src/includes.h"
#include "src/raycasting.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;



vec2 playerPosition(0.0f, 0.0f);
float playerViewAngle = 0.0f;

unordered_map<int, bool> keyMap = {};
utils::Line levelData[256];

// Keyboard presses to monitor.
const std::array<int, 16> monitoredKeys = { // 16 long to cover more keys added later, without having to change that value.
	GLFW_KEY_W, GLFW_KEY_S,
	GLFW_KEY_A, GLFW_KEY_D,
	GLFW_KEY_SPACE,
	GLFW_KEY_LEFT_CONTROL
};

int main() {
	//OpenGL error #1282 again. Happens here and later at glGetError().
	//I dislike this, I just want to do cool raycasting rendering :(
	

	FrameBuffer frameBuffer = FrameBuffer(display::screenWidth, display::screenHeight);
	
	GLFWwindow* Window = render::initializeWindow(display::screenWidth, display::screenHeight, "Window");
	utils::GLErrorcheck("Window Creation", true);


	GLuint frameBufferTexture = render::createTexture();
	GLuint shaderProgram = render::loadShaders();

	//Not any of these causing the 1282
	glViewport(0, 0, display::screenWidth, display::screenHeight);
	glDisable(GL_DEPTH_TEST);
	GLuint VAO = render::getVAO();

	utils::GLErrorcheck("Initialisation", true);


	// Initialize keyMap for input tracking
	for (int key : monitoredKeys) {
		keyMap[key] = false;
	}

	while (!glfwWindowShouldClose(Window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		glfwPollEvents();

		// Get inputs for this frame
		for (int key : monitoredKeys) {
			int keyState = glfwGetKey(Window, key);
			if (keyState == GLFW_PRESS) {
				keyMap[key] = true;
			} else if (keyState == GLFW_RELEASE) {
				keyMap[key] = false;
			}
		}

		if (keyMap[GLFW_KEY_A]) {
			playerViewAngle -= player::turnSpeed;
		} else if (keyMap[GLFW_KEY_D]) {
			playerViewAngle += player::turnSpeed;
		}
		playerViewAngle = fmod(playerViewAngle, 360.0f); // -180 -> 180 degrees

		playerPosition = physics::playerMove(playerPosition, playerViewAngle);

		// Update Texture with current frameBuffer contents

		// Render to screen
		glUseProgram(shaderProgram);
		utils::GLErrorcheck("ShaderProgram Binding", true);

		/*
		#Previous implementation in PyOpenGL, in another project
		glBindVertexArray(VAO_QUAD)
		glBindTextureUnit(0, TCB_SCENE)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, None)
		glBindVertexArray(0)
		*/
		render::updateTexture(frameBufferTexture, frameBuffer);
		utils::GLErrorcheck("TextureUpd", true);

		//utils::saveTextureToFile(frameBufferTexture, display::screenWidth, display::screenHeight, "output.png");
		//utils::pause();


		GLuint frameBufferShaderLoc = glGetUniformLocation(shaderProgram, "frameBufferID");
		glUniform1i(frameBufferShaderLoc, 0);

		// Bind the texture to render
		glBindVertexArray(VAO);
		glBindTextureUnit(0, frameBufferTexture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Rendering", true);
	}

	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;
}
