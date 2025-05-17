#define STB_IMAGE_IMPLEMENTATION
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "src/includes.h"
#include "src/stageLoader.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;


//Images to be used in the UI.
std::array<std::string, 32> UIImageNames = {
	"ui-health", "ui-energy"
};

//Symbols to be used in the UI.
std::array<std::string, 32> symbolNames = {
	"symbol_0", "symbol_1",
	"symbol_2", "symbol_3",
	"symbol_4", "symbol_5",
	"symbol_6", "symbol_7",
	"symbol_8", "symbol_9",
	"symbol_-"
};

// Keyboard presses to monitor.
std::array<int, 16> monitoredKeys = { //16 should cover necessary keys.
	GLFW_KEY_W, GLFW_KEY_S,
	GLFW_KEY_A, GLFW_KEY_D,
	GLFW_KEY_E, GLFW_KEY_F,
	GLFW_KEY_SPACE,
	GLFW_KEY_LEFT_SHIFT, GLFW_KEY_LEFT_CONTROL,
	GLFW_KEY_1, GLFW_KEY_C,
	GLFW_KEY_ESCAPE
};




GLuint renderedFrameID, depthSSBO;
glm::ivec2 currentScreenRes;
unordered_map<int, bool> keyMap = {};
bool headLampEnabled = false;
int tick = 0;


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentScreenRes = glm::ivec2(width, height);
}





int main() {
	try { //Catch exceptions
	Player player = Player(playerConfig::PLAYER_START_POSITION, playerConfig::PLAYER_START_ANGLE);
	std::array<int, constants::MAX_FLAGS> flags;
	std::array<utils::Visplane, constants::MAX_VISPLANES> visplaneData;
	std::array<utils::Wall, constants::MAX_WALLS> wallData;
	std::array<utils::Sprite, constants::MAX_SPRITES> spriteData;
	std::array<utils::Light, constants::MAX_LIGHTS> lightData;
	std::array<utils::LogicGate, constants::MAX_GATES> logicGates;

	std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;

	stageLoader::loadStage(
		playerConfig::STAGE_NAME,
		&visplaneData, &wallData,
		&spriteData, &lightData,
		&logicGates, &flags,
		&textureNames
	);


	double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
	currentScreenRes = display::SCREEN_RESOLUTION;


	GLFWwindow* Window = render::initializeWindow(currentScreenRes.x, currentScreenRes.y, "Raycasting-Renderer/GPU");
	glfwSetFramebufferSizeCallback(Window, framebuffer_size_callback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	glEnable(GL_BLEND);

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);




	renderedFrameID = render::createGLImage2D(display::RENDER_RESOLUTION.x, display::RENDER_RESOLUTION.y);
	GLuint textureArrayEnvironment = render::createTexture2DArray(textureNames);
	GLuint textureArrayUI = render::createTexture2DArray(UIImageNames);
	GLuint textureArrayNumeric = render::createTexture2DArray(symbolNames);
	GLuint skyboxTextureID = render::loadGLTexture2D("skybox", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);

	render::createConstUBO();

	GLuint visplaneUBO = render::createVisplaneUBO();
	GLuint wallUBO = render::createWallUBO();
	GLuint lightSSBO = render::createLightSSBO();
	GLuint spriteSSBO = render::createSpriteSSBO();


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


	utils::GLErrorcheck("Initialisation", true);



	// Initialize keyMap for input tracking
	for (int key : monitoredKeys) {
		keyMap[key] = false;
	}
	bool interactKey = false;
	int lightFlickerRNG, FPS = 0;

	while (!glfwWindowShouldClose(Window)) {
		tick++;
		double frameStart = glfwGetTime();
		glfwPollEvents();

		// Get inputs for this frame
		for (int key : monitoredKeys) {
			int keyState = glfwGetKey(Window, key);
			if (keyState == GLFW_PRESS) {
				if (key == GLFW_KEY_F && !keyMap[GLFW_KEY_F]) {
					headLampEnabled = !headLampEnabled;
				}
				interactKey = (key == GLFW_KEY_E) && (!keyMap[GLFW_KEY_E]);

				keyMap[key] = true;


			} else if (keyState == GLFW_RELEASE) {
				keyMap[key] = false;
			}
		}


		if (keyMap[GLFW_KEY_ESCAPE]) {
			break; //Quit
		}

		if (keyMap[GLFW_KEY_1]) {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);			
		} else {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
		}

		if (keyMap[GLFW_KEY_LEFT_CONTROL]) {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_CROUCH;
			player.touchingFloor = false;
		} else {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_STAND;
		}



		float rayAngle = (keyMap[GLFW_KEY_C]) ? display::MAX_RAY_ANGLE/display::ZOOM_MULT : display::MAX_RAY_ANGLE;

		double cursorXDelta = cursorXPos - cursorXPosPrev;
		double cursorYDelta = cursorYPos - cursorYPosPrev;
		player.viewAngle += cursorXDelta * (playerConfig::TURN_SPEED_CURS / display::ZOOM_MULT);
		player.viewAngle = utils::angleClamp(player.viewAngle);
		if (dev::LOCK_VLOOK < 1) {
			double dY = cursorYDelta * (playerConfig::TURN_SPEED_CURS / display::ZOOM_MULT);
			player.vLook = glm::clamp(float(player.vLook+dY), -22.5f, 22.5f);
		}


		//Update logic states.
		for (int index=0; index<constants::MAX_GATES; index++) {
			LogicGate gate = logicGates[index];
			if (gate.gateType == G_INVALID) {continue;}
			gate.evaluateState();
			logicGates[index] = gate;
		}
		physics::updateSpecials(&wallData, &visplaneData, &player, keyMap, interactKey);


		physics::playerMove(&player, keyMap, &wallData, &spriteData, &visplaneData);
		float viewBob = (dev::VIEW_BOB_DISABLE > 0) ? 0.0f : render::viewBob(tick, player);
		player.cameraPosition = player.position + glm::vec3(0.0f, 0.0f, (player.height/3.0f) + viewBob);



		glm::vec4 tintData = render::manageScreenTint(0, player.state);



		//Update Dynamic UBOs.
		render::updateVisplaneUBO(visplaneUBO, &visplaneData);
		render::updateWallUBO(wallUBO, &wallData);
		render::updateSpriteSSBO(spriteSSBO, &spriteData);
		render::updateLightSSBO(lightSSBO, &lightData);
		utils::GLErrorcheck("Updating UBOs", true);


		if (headLampEnabled) {
			lightFlickerRNG = utils::RNGc();
		}


		//Update resolution
		glViewport(0, 0, display::RENDER_RESOLUTION.x, display::RENDER_RESOLUTION.y);

		//Environment Shader.
		glUseProgram(envShader);
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glBindTextureUnit(0, textureArrayEnvironment);
		glBindTextureUnit(1, skyboxTextureID);


		//Player Data
		GLuint playerPosLocation = glGetUniformLocation(envShader, "playerPosition");
		GLuint playerAngleLocation = glGetUniformLocation(envShader, "playerViewAngle");
		GLuint playerRollLocation = glGetUniformLocation(envShader, "playerViewRoll");
		GLuint playerPitchLocation = glGetUniformLocation(envShader, "playerViewPitch");
		GLuint zoomLocation = glGetUniformLocation(envShader, "zoom");
		glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1f(playerRollLocation, player.viewRoll);
		glUniform1f(playerPitchLocation, player.viewPitch);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);

		//Debug
		GLuint uvLocation = glGetUniformLocation(envShader, "drawUV");
		glUniform1i(uvLocation, dev::DRAW_UV);

		//Headlamp
		GLuint lightLocation = glGetUniformLocation(envShader, "headLampEnabled");
		GLuint lightFlickerLocation = glGetUniformLocation(envShader, "headLampFlicker");
		glUniform1i(lightLocation, headLampEnabled);
		glUniform1i(lightFlickerLocation, lightFlickerRNG);

		//Sun
		GLuint sunDirLocation = glGetUniformLocation(envShader, "sunDirection");
		GLuint sunColourLocation = glGetUniformLocation(envShader, "sunColour");
		glUniform3f(sunDirLocation, display::SUN_DIRECTION.x, display::SUN_DIRECTION.y, display::SUN_DIRECTION.z);
		glUniform3f(sunColourLocation, display::SUN_COLOUR.x, display::SUN_COLOUR.y, display::SUN_COLOUR.z);


		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Environment Shader", true);



		//Sprite Shader.
		glUseProgram(spriteShader);
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindTextureUnit(0, textureArrayEnvironment);


		//Player Data
		playerPosLocation = glGetUniformLocation(spriteShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(spriteShader, "playerViewAngle");
		playerRollLocation = glGetUniformLocation(spriteShader, "playerViewRoll");
		playerPitchLocation = glGetUniformLocation(spriteShader, "playerViewPitch");
		zoomLocation = glGetUniformLocation(spriteShader, "zoom");
		glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1f(playerRollLocation, player.viewRoll);
		glUniform1f(playerPitchLocation, player.viewPitch);
		glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);

		//Debug
		uvLocation = glGetUniformLocation(spriteShader, "drawUV");
		glUniform1i(uvLocation, dev::DRAW_UV);

		//Headlamp
		lightLocation = glGetUniformLocation(spriteShader, "headLampEnabled");
		lightFlickerLocation = glGetUniformLocation(spriteShader, "headLampFlicker");
		glUniform1i(lightLocation, headLampEnabled);
		glUniform1i(lightFlickerLocation, lightFlickerRNG);

		//Sun
		sunDirLocation = glGetUniformLocation(spriteShader, "sunDirection");
		sunColourLocation = glGetUniformLocation(spriteShader, "sunColour");
		glUniform3f(sunDirLocation, display::SUN_DIRECTION.x, display::SUN_DIRECTION.y, display::SUN_DIRECTION.z);
		glUniform3f(sunColourLocation, display::SUN_COLOUR.x, display::SUN_COLOUR.y, display::SUN_COLOUR.z);


		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Sprite Shader", true);

		

		//UI Shader.
		if (!(dev::NO_INTERFACE > 0)) {
			//Update resolution
			glViewport(0, 0, display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);


			glUseProgram(uiShader);
			glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

			glBindTextureUnit(0, textureArrayEnvironment); //World Textures
			glBindTextureUnit(1, textureArrayUI); //UI Textures
			glBindTextureUnit(2, textureArrayNumeric); //0-9 Textures.


			//Player Data.
			playerPosLocation = glGetUniformLocation(uiShader, "playerPosition");
			playerAngleLocation = glGetUniformLocation(uiShader, "playerViewAngle");
			zoomLocation = glGetUniformLocation(uiShader, "zoom");
			GLuint vignetteColourLocation = glGetUniformLocation(uiShader, "screenTint");
			GLuint healthLocation = glGetUniformLocation(uiShader, "health");
			GLuint energyLocation = glGetUniformLocation(uiShader, "energy");
			glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
			glUniform1f(playerAngleLocation, player.viewAngle);
			glUniform1i(zoomLocation, keyMap[GLFW_KEY_C]);
			glUniform4f(vignetteColourLocation, tintData.x, tintData.y, tintData.z, tintData.w);
			glUniform1i(healthLocation, player.health);
			glUniform1i(energyLocation, player.energy);

			//Assorted other data.
			GLuint screenResLocation = glGetUniformLocation(uiShader, "screenResolution");
			GLuint freqLocation = glGetUniformLocation(uiShader, "FPS");
			GLuint showFreqLocation = glGetUniformLocation(uiShader, "showFreq");
			glUniform2i(screenResLocation, currentScreenRes.x, currentScreenRes.y);
			glUniform1i(freqLocation, FPS);
			glUniform1i(showFreqLocation, dev::SHOW_FREQ);


			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			utils::GLErrorcheck("UI Shader", true);
		}
		


		//Update resolution
		glViewport(0, 0, currentScreenRes.x, currentScreenRes.y);

		//Display Shader and update screen.
		glUseProgram(displayShader);
		glBindTextureUnit(0, renderedFrameID);


		//Assorted other data.
		GLuint screenResLocation = glGetUniformLocation(displayShader, "screenResolution");
		GLuint smoothingLocation = glGetUniformLocation(displayShader, "smoothingEnabled");
		glUniform2i(screenResLocation, currentScreenRes.x, currentScreenRes.y);
		glUniform1i(smoothingLocation, dev::SMOOTHING_ENABLED);



		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Display Shader", true);

		while (glfwGetTime() - frameStart < constants::DT) {}
		if (dev::SHOW_FREQ > 0) {
			double totalTime = (glfwGetTime() - frameStart);
			FPS = floor(1/totalTime);
		}

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
