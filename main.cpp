#define STB_IMAGE_IMPLEMENTATION
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "src/includes.h"
#include "src/raycasting.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;


const double wait_time = 1.0f / display::maxFPS;


unordered_map<int, bool> keyMap = {};

// Keyboard presses to monitor.
const std::array<int, 16> monitoredKeys = { // 16 long to cover more keys added later, without having to change that value.
	GLFW_KEY_W, GLFW_KEY_S,
	GLFW_KEY_A, GLFW_KEY_D,
	GLFW_KEY_E, GLFW_KEY_Q,
	GLFW_KEY_SPACE,
	GLFW_KEY_LEFT_CONTROL
};


std::array<utils::Wall, 128> prepLines() {
	std::array<utils::Wall, 128> wallData;

	wallData[0] = Wall(glm::vec2(-1, -1), glm::vec2( 1, -1), glm::vec3(255, 000, 255));
	wallData[1] = Wall(glm::vec2( 1,  1), glm::vec2(-1, -1), glm::vec3(000, 255, 255));


	wallData[2] = Wall(glm::vec2(-8, -8), glm::vec2( 0, -8), glm::vec3(255, 000, 000));
	wallData[3] = Wall(glm::vec2( 0, -8), glm::vec2( 8, -8), glm::vec3(255, 000, 000));

	wallData[4] = Wall(glm::vec2(-8, -8), glm::vec2(-8,  0), glm::vec3(255, 000, 000));
	wallData[5] = Wall(glm::vec2(-8,  0), glm::vec2(-8,  8), glm::vec3(255, 000, 000));

	wallData[6] = Wall(glm::vec2(-8,  8), glm::vec2( 0,  8), glm::vec3(255, 000, 000));
	wallData[7] = Wall(glm::vec2( 0,  8), glm::vec2( 8,  8), glm::vec3(255, 000, 000));

	wallData[8] = Wall(glm::vec2( 8, -8), glm::vec2( 8,  -0.5), glm::vec3(255, 000, 000));
	wallData[9] = Wall(glm::vec2( 8,  0.5), glm::vec2( 8,  8), glm::vec3(255, 000, 000));

	return wallData;
}


int main() {
	std::array<utils::Wall, 128> wallData = prepLines();
	FrameBuffer frameBuffer = FrameBuffer(display::screenWidth, display::screenHeight);
	Player player = Player(playerConfig::playerStartPos, playerConfig::playerStartAngle);



	const char* texturePath = "src/textures/texture.jpg";
	int width, height, channels;
	unsigned char* textureData = stbi_load(texturePath, &width, &height, &channels, 0);

	if (textureData == nullptr) {
		std::cout << stbi_failure_reason() << std::endl;
		raise("Failed to load image");
		return -1;
	}



	GLFWwindow* Window = render::initializeWindow(display::screenWidth, display::screenHeight, "Window");
	utils::GLErrorcheck("Window Creation", true);


	GLuint frameBufferTexture = render::createTexture();
	GLuint shaderProgram = render::loadShaders();


	glViewport(0, 0, display::screenWidth, display::screenHeight);
	glDisable(GL_DEPTH_TEST);
	GLuint VAO = render::getVAO();

	utils::GLErrorcheck("Initialisation", true);

	double frame_start;


	// Initialize keyMap for input tracking
	for (int key : monitoredKeys) {
		keyMap[key] = false;
	}

	while (!glfwWindowShouldClose(Window)) {
		frame_start = glfwGetTime();
		glClear(GL_COLOR_BUFFER_BIT);
		frameBuffer.clearBuffer();
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

		if (keyMap[GLFW_KEY_Q]) {
			player.viewAngle -= playerConfig::turnSpeed;
		}
		if (keyMap[GLFW_KEY_E]) {
			player.viewAngle += playerConfig::turnSpeed;
		}
		player.viewAngle = utils::angleClamp(player.viewAngle);

		player = physics::playerMove(player, keyMap, &wallData);



		//Update the pixels and Raycast.
		raycasting::checkRays(&frameBuffer, player, &wallData, textureData);

		render::updateTexture(frameBufferTexture, frameBuffer);
		utils::GLErrorcheck("TextureUpd", true);



		//Shader and Screen
		glUseProgram(shaderProgram);
		utils::GLErrorcheck("ShaderProgram Binding", true);


		GLuint frameBufferShaderLoc = glGetUniformLocation(shaderProgram, "frameBufferID");
		glUniform1i(frameBufferShaderLoc, 0);

		// Bind the texture to render
		glBindVertexArray(VAO);
		glBindTextureUnit(0, frameBufferTexture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Rendering", true);


		while (glfwGetTime() - frame_start < wait_time) {}
		double totalTime = (glfwGetTime() - frame_start);
		//std::cout << "FPS " << 1/totalTime << endl;
	}

	glfwDestroyWindow(Window);
	glfwTerminate();
	stbi_image_free(textureData);
	return 0;
}
