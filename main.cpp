#define STB_IMAGE_IMPLEMENTATION
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "src/includes.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;


const double wait_time = 1.0f / display::maxFPS;


unordered_map<int, bool> keyMap = {};

std::array<std::string, 32> textureNames = {
	"a",
	"b",
	"c",
	"s_t_a_r_e",
	"tabs=fish",
};

// Keyboard presses to monitor.
const std::array<int, 16> monitoredKeys = { // 16 long to cover more keys added later, without having to change that value.
	GLFW_KEY_W, GLFW_KEY_S,
	GLFW_KEY_A, GLFW_KEY_D,
	GLFW_KEY_E, GLFW_KEY_Q,
	GLFW_KEY_SPACE,
	GLFW_KEY_LEFT_SHIFT,
	GLFW_KEY_1, GLFW_KEY_C,
	GLFW_KEY_ESCAPE,
};


std::array<utils::Wall, 256> prepWalls() {
	std::array<utils::Wall, 256> wallData;

	wallData[0*2] = Wall(glm::vec2(-1, -1), glm::vec2( 1, -1), 4);
	wallData[1*2] = Wall(glm::vec2( 1,  1), glm::vec2(-1, -1), 4);

	wallData[2*2] = Wall(glm::vec2( 0, -8), glm::vec2(-8, -8), 0);
	wallData[3*2] = Wall(glm::vec2( 8, -8), glm::vec2( 0, -8), 0);

	wallData[4*2] = Wall(glm::vec2(-8, -8), glm::vec2(-8,  0), 0);
	wallData[5*2] = Wall(glm::vec2(-8,  0), glm::vec2(-8,  8), 0);

	wallData[6*2] = Wall(glm::vec2(-8,  8), glm::vec2( 0,  8), 0);
	wallData[7*2] = Wall(glm::vec2( 0,  8), glm::vec2( 8,  8), 0);

	wallData[8*2] = Wall(glm::vec2( 8, -0.5), glm::vec2( 8, -8), 0);
	wallData[9*2] = Wall(glm::vec2( 8,  8), glm::vec2( 8,  0.5), 0);

	return wallData;
}


std::array<utils::Sprite, 32> prepSprites() {
	std::array<utils::Sprite, 32> spriteData;

	spriteData[0] = Sprite(glm::vec2(5, 5), 1.0f, 3);

	return spriteData;

}


std::array<utils::Light, 32> prepLights() {
	std::array<utils::Light, 32> lightData;

	lightData[0] = Light(glm::vec3(0, 0, 0), glm::vec3(255, 0, 255), 1.0f);

	return lightData;
}


GLuint frameTextureID, depthSSBO;


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);

    GLuint newFrameTextureID = render::createTexture(width, height);

    glDeleteTextures(1, &frameTextureID);
    frameTextureID = newFrameTextureID;
	depthSSBO = render::createDepthSSBO(display::screenWidth);
}


int main() {
	try { //Catch exceptions

	std::array<utils::Wall, 256> wallData = prepWalls();
	std::array<utils::Sprite, 32> spriteData = prepSprites();
	std::array<utils::Light, 32> lightData = prepLights();
	Player player = Player(playerConfig::playerStartPos, playerConfig::playerStartAngle);


	double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;


	GLFWwindow* Window = render::initializeWindow(display::screenWidth, display::screenHeight, "Raycasting-Renderer");
	glfwSetFramebufferSizeCallback(Window, framebuffer_size_callback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);




	frameTextureID = render::createTexture(display::screenWidth, display::screenHeight);
	GLuint textureArray = render::createTextureArray(textureNames);
	render::createConstUBO();
	render::createWallUBO(&wallData);
	render::createLightUBO(&lightData);
	GLuint spriteUBO = render::createSpriteUBO();
	depthSSBO = render::createDepthSSBO(display::screenWidth);


	//Visplane Shader (Roof and Floor).
	GLuint visplaneShader = render::createShaderProgram("visplanes", false);

	//Wall Shader
	GLuint wallShader = render::createShaderProgram("walls", false);

	//Sprite Shader
	GLuint spriteShader = render::createShaderProgram("sprites", false);

	//uiShader
	GLuint uiShader = render::createShaderProgram("interface", false);

	//Display Shader
	GLuint displayShader = render::createShaderProgram("display", true);



	glViewport(0, 0, display::screenWidth, display::screenHeight);
	glDisable(GL_DEPTH_TEST);
	GLuint VAO = render::getVAO();

	utils::GLErrorcheck("Initialisation", true);



	double frame_start, cursorXDelta;
	GLint topIndexLocation, lowIndexLocation, zoomLocation, uvLocation, playerPosLocation, playerAngleLocation;

	// Initialize keyMap for input tracking
	for (int key : monitoredKeys) {
		keyMap[key] = false;
	}

	while (!glfwWindowShouldClose(Window)) {
		frame_start = glfwGetTime();
		//glClear(GL_COLOR_BUFFER_BIT);
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


		if (keyMap[GLFW_KEY_ESCAPE]) {
			break; //Quit
		}
		if (keyMap[GLFW_KEY_Q]) {
			player.viewAngle -= playerConfig::turnSpeedKB;
		}
		if (keyMap[GLFW_KEY_E]) {
			player.viewAngle += playerConfig::turnSpeedKB;
		}

		if (keyMap[GLFW_KEY_1]) {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);			
		} else {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
		}

		float rayAngle = (keyMap[GLFW_KEY_C]) ? display::maxRayAngle/display::zoomFactor : display::maxRayAngle;

		cursorXDelta = cursorXPos - cursorXPosPrev;
		player.viewAngle += cursorXDelta * (playerConfig::turnSpeedCursor * (rayAngle/display::maxRayAngle));
		player.viewAngle = utils::angleClamp(player.viewAngle);

		player = physics::playerMove(player, keyMap, &wallData);



		//Update Sprites UBO.
		render::updateSpriteUBO(&spriteUBO, &spriteData);


		//Visplanes Shader.
		glUseProgram(visplaneShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		topIndexLocation = glGetUniformLocation(visplaneShader, "topIndex");
		lowIndexLocation = glGetUniformLocation(visplaneShader, "lowIndex");
		playerPosLocation = glGetUniformLocation(visplaneShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(visplaneShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(visplaneShader, "zoom");
		uvLocation = glGetUniformLocation(visplaneShader, "drawUV");
		
		glUniform1i(topIndexLocation, display::topIndex);
		glUniform1i(lowIndexLocation, display::lowIndex);
		glUniform2f(playerPosLocation, player.position.x, player.position.y);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
		glUniform1i(uvLocation, dev::drawUV);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Visplane Shader", true);


		//Wall Shader.
		glUseProgram(wallShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		playerPosLocation = glGetUniformLocation(wallShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(wallShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(wallShader, "zoom");
		uvLocation = glGetUniformLocation(wallShader, "drawUV");
		
		glUniform2f(playerPosLocation, player.position.x, player.position.y);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
		glUniform1i(uvLocation, dev::drawUV);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Walls Shader", true);


		//Sprite Shader.
		glUseProgram(spriteShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		playerPosLocation = glGetUniformLocation(spriteShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(spriteShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(spriteShader, "zoom");
		uvLocation = glGetUniformLocation(spriteShader, "drawUV");
		
		glUniform2f(playerPosLocation, player.position.x, player.position.y);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
		glUniform1i(uvLocation, dev::drawUV);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Sprite Shader", true);


		
		//UI Shader.
		glUseProgram(uiShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		playerPosLocation = glGetUniformLocation(uiShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(uiShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(uiShader, "zoom");
		
		glUniform2f(playerPosLocation, player.position.x, player.position.y);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("UI Shader", true);
		


		//Display Shader and update screen.
		glUseProgram(displayShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Display Shader", true);


		while (glfwGetTime() - frame_start < wait_time) {}
		if (dev::printFPS == 1) {double totalTime = (glfwGetTime() - frame_start);std::cout << "FPS " << 1/totalTime << endl;}


		cursorXPosPrev = cursorXPos;
		cursorYPosPrev = cursorYPos;
	}

	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;


	//Catch exceptions.
	} catch (const std::exception& e) {
		std::cerr << "An exception was thrown: " << e.what() << std::endl;
		pause();
		return -1;
	} catch (...) {
		std::cerr << "An unspecified exception was thrown." << std::endl;
		pause();
		return -1;
	}
}
