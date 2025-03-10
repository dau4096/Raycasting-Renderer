#define STB_IMAGE_IMPLEMENTATION
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "src/includes.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;



unordered_map<int, bool> keyMap = {};

std::array<std::string, 32> textureNames = {
	"a", "b", "c",
	"s_t_a_r_e",
	"tabs=fish",
	"piloten",
	"mus2", "osa"
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




//TEMPORARY DATA SETUP. REPLACE WITH FILE LOADING.
std::array<utils::Visplane, constants::MAX_VISPLANES> prepVisplanes() {
	std::array<utils::Visplane, constants::MAX_VISPLANES> visplaneData;

	visplaneData[0] = Visplane(vec2(-10, -10), vec2(10, 10), -1.0f, 2);
	visplaneData[1] = Visplane(vec2(-8, -8), vec2( 8, -12), 1.8f, 3);

	return visplaneData;
}


std::array<utils::Wall, constants::MAX_WALLS> prepWalls() {
	std::array<utils::Wall, constants::MAX_WALLS> wallData;

	wallData[0] = Wall(glm::vec2(-1, -1), glm::vec2( 1, -1), -1.0f, 3.0f, 4);
	wallData[1] = Wall(glm::vec2( 1,  1), glm::vec2(-1, -1), -1.0f, 2.0f, 4);

	wallData[2] = Wall(glm::vec2( 0, -8), glm::vec2(-8, -8), -1.0f, 2.0f, 2);
	wallData[3] = Wall(glm::vec2( 8, -8), glm::vec2( 0, -8), -1.0f, 2.0f, 2);

	wallData[4] = Wall(glm::vec2(-8, -8), glm::vec2(-8,  0), -1.0f, 1.0f, 0);
	wallData[5] = Wall(glm::vec2(-8,  0), glm::vec2(-8,  8), -1.0f, 1.0f, 0);

	wallData[6] = Wall(glm::vec2(-8,  8), glm::vec2( 0,  8), -1.0f, 1.0f, 0);
	wallData[7] = Wall(glm::vec2( 0,  8), glm::vec2( 8,  8), -1.0f, 1.0f, 0);

	wallData[8] = Wall(glm::vec2( 8, -0.5), glm::vec2( 8, -8), -1.0f, 1.0f, 0);
	wallData[9] = Wall(glm::vec2( 8,  8), glm::vec2( 8,  0.5), -1.0f, 1.0f, 0);


	wallData[10] = Wall(glm::vec2(-8, -8), glm::vec2(-8, -12), -1.0f, 2.0f, 2);
	wallData[11] = Wall(glm::vec2( 8, -8), glm::vec2( 8, -12), -1.0f, 2.0f, 2);


	return wallData;
}


std::array<utils::Sprite, constants::MAX_SPRITES> prepSprites() {
	std::array<utils::Sprite, constants::MAX_SPRITES> spriteData;

	spriteData[0] = Sprite(glm::vec2( 5,  5), 1.0f, 5);
	spriteData[1] = Sprite(glm::vec2(-4,  4), 1.0f, 4);

	return spriteData;

}


std::array<utils::Light, constants::MAX_LIGHTS> prepLights() {
	std::array<utils::Light, constants::MAX_LIGHTS> lightData;

	lightData[0] = Light(glm::vec3(-5, 0, -5), glm::vec3(1.0f, 1.0f, 1.0f), 10.0f);
	lightData[1] = Light(glm::vec3( 5, 0,  5), glm::vec3(1.0f, 0.0f, 1.0f), 5.0f);

	return lightData;
}



GLuint frameTextureID, depthSSBO;
glm::ivec2 currentScreenRes;


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);

	currentScreenRes = glm::ivec2(width, height);

	depthSSBO = render::createDepthSSBO(display::RENDER_RESOLUTION.x);
}


int main() {
	try { //Catch exceptions

	std::array<utils::Visplane, constants::MAX_VISPLANES> visplaneData = prepVisplanes();
	std::array<utils::Wall, constants::MAX_WALLS> wallData = prepWalls();
	std::array<utils::Sprite, constants::MAX_SPRITES> spriteData = prepSprites();
	std::array<utils::Light, constants::MAX_LIGHTS> lightData = prepLights();
	Player player = Player(playerConfig::PLAYER_START_POSITION, playerConfig::PLAYER_START_ANGLE);


	double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
	currentScreenRes = display::SCREEN_RESOLUTION;


	GLFWwindow* Window = render::initializeWindow(currentScreenRes.x, currentScreenRes.y, "Raycasting-Renderer");
	glfwSetFramebufferSizeCallback(Window, framebuffer_size_callback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);




	frameTextureID = render::createTexture(display::RENDER_RESOLUTION.x, display::RENDER_RESOLUTION.y);
	GLuint textureArray = render::createTextureArray(textureNames);

	render::createConstUBO();

	GLuint visplaneUBO = render::createVisplaneUBO();
	GLuint wallUBO = render::createWallUBO();
	GLuint lightSSBO = render::createLightSSBO();
	GLuint spriteSSBO = render::createSpriteSSBO();

	depthSSBO = render::createDepthSSBO(display::RENDER_RESOLUTION.x);


	//Environment shader
	GLuint envShader = render::createShaderProgram("environment", false);

	//Sprite Shader
	GLuint spriteShader = render::createShaderProgram("sprites", false);

	//uiShader
	GLuint uiShader = render::createShaderProgram("interface", false);

	//Display Shader
	GLuint displayShader = render::createShaderProgram("display");



	glViewport(0, 0, currentScreenRes.x, currentScreenRes.y);
	glDisable(GL_DEPTH_TEST);
	GLuint VAO = render::getVAO();


	//Update Visplane and Wall UBOs
	//Will be made dynamic later when moving environment objects are added.
	render::updateVisplaneUBO(visplaneUBO, &visplaneData);
	render::updateWallUBO(wallUBO, &wallData);


	utils::GLErrorcheck("Initialisation", true);



	double frameStart, cursorXDelta;
	GLint topIndexLocation, lowIndexLocation, zoomLocation, uvLocation, playerPosLocation, playerAngleLocation;

	// Initialize keyMap for input tracking
	for (int key : monitoredKeys) {
		keyMap[key] = false;
	}

	while (!glfwWindowShouldClose(Window)) {
		frameStart = glfwGetTime();
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
			player.position.z -= playerConfig::MOVE_SPEED_BASE;
		}
		if (keyMap[GLFW_KEY_E]) {
			player.position.z += playerConfig::MOVE_SPEED_BASE;
		}

		if (keyMap[GLFW_KEY_1]) {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);			
		} else {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
		}



		float rayAngle = (keyMap[GLFW_KEY_C]) ? display::MAX_RAY_ANGLE/display::ZOOM_MULT : display::MAX_RAY_ANGLE;

		cursorXDelta = cursorXPos - cursorXPosPrev;
		player.viewAngle += cursorXDelta * (playerConfig::TURN_SPEED_CURS / display::ZOOM_MULT);
		player.viewAngle = utils::angleClamp(player.viewAngle);


		player = physics::playerMove(player, keyMap, &wallData, &spriteData, &visplaneData);


		//Update Dynamic UBOs.
		render::updateSpriteSSBO(spriteSSBO, &spriteData);
		render::updateLightSSBO(lightSSBO, &lightData);
		utils::GLErrorcheck("Updating UBOs", true);




		//Environment Shader.
		glUseProgram(envShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		playerPosLocation = glGetUniformLocation(envShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(envShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(envShader, "zoom");
		uvLocation = glGetUniformLocation(envShader, "drawUV");
		
		glUniform3f(playerPosLocation, player.position.x, player.position.y, player.position.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
		glUniform1i(uvLocation, dev::DRAW_UV);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Environment Shader", true);


		//Sprite Shader.
		glUseProgram(spriteShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		glBindTextureUnit(0, textureArray);

		playerPosLocation = glGetUniformLocation(spriteShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(spriteShader, "playerViewAngle");
		zoomLocation = glGetUniformLocation(spriteShader, "zoom");
		uvLocation = glGetUniformLocation(spriteShader, "drawUV");
		
		glUniform3f(playerPosLocation, player.position.x, player.position.y, player.position.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
		glUniform1i(uvLocation, dev::DRAW_UV);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Sprite Shader", true);

		
		//UI Shader.
		if (dev::NO_INTERFACE <= 0) {
			glUseProgram(uiShader);
			glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

			glBindTextureUnit(0, textureArray);

			playerPosLocation = glGetUniformLocation(uiShader, "playerPosition");
			playerAngleLocation = glGetUniformLocation(uiShader, "playerViewAngle");
			zoomLocation = glGetUniformLocation(uiShader, "zoom");
			
			glUniform3f(playerPosLocation, player.position.x, player.position.y, player.position.z);
			glUniform1f(playerAngleLocation, player.viewAngle);
			glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			utils::GLErrorcheck("UI Shader", true);
		}
		


		//Display Shader and update screen.
		glUseProgram(displayShader);
		glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, depthSSBO);

		GLuint screenResLoc = glGetUniformLocation(displayShader, "screenResolution");
		glUniform2i(screenResLoc, currentScreenRes.x, currentScreenRes.y);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Display Shader", true);


		while (glfwGetTime() - frameStart < static_cast<double>(constants::DT)) {}
		if (dev::SHOW_FREQ > 0) {double totalTime = (glfwGetTime() - frameStart);std::cout << "FPS " << 1/totalTime << endl;}


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
