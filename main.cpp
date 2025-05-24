#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image_write.h"
#include "src/includes.h"
#include "src/global.h"
#include "src/loader.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;


//Images to be used in the UI.
std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> UIImageNames = {
	"ui-health", "ui-energy"
};

//Symbols to be used in the UI.
std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> symbolNames = {
	"symbol_0", "symbol_1",
	"symbol_2", "symbol_3",
	"symbol_4", "symbol_5",
	"symbol_6", "symbol_7",
	"symbol_8", "symbol_9",
	"symbol_DOT", "symbol_DASH",
	"symbol_EMARK", "symbol_QMARK",
	"symbol_COMMA", "symbol_QUOTE",
	"symbol_FSLASH", "symbol_COLON",
	"symbol_SEMICOLON", "symbol_AND",
	"symbol_OPNBRACKET", "symbol_CLSBRACKET",
	"symbol_CARET", "symbol_UNKNOWN",
	"symbol_A", "symbol_B",
	"symbol_C", "symbol_D",
	"symbol_E", "symbol_F",
	"symbol_G", "symbol_H",
	"symbol_I", "symbol_J",
	"symbol_K", "symbol_L",
	"symbol_M", "symbol_N",
	"symbol_O", "symbol_P",
	"symbol_Q", "symbol_R",
	"symbol_S", "symbol_T",
	"symbol_U", "symbol_V",
	"symbol_W", "symbol_X",
	"symbol_Y", "symbol_Z"
};



GLuint renderedFrameID, depthUBO;
glm::ivec2 currentScreenRes;
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
	Player player;
	std::array<utils::Visplane, constants::MAX_VISPLANES> visplaneData;
	std::array<utils::Wall, constants::MAX_WALLS> wallData;
	std::array<utils::Sprite, constants::MAX_SPRITES> spriteData;
	std::array<utils::Light, constants::MAX_LIGHTS> lightData;
	std::array<utils::TextObject, constants::MAX_TEXT_OBJECTS> textObjectData;
	std::array<utils::LogicGate, constants::MAX_GATES> logicGates;
	std::array<int, constants::MAX_FLAGS> flags;

	std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;

	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		&visplaneData, &wallData,
		&spriteData, &lightData,
		&textObjectData,
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
	GLuint textureArrayUI = render::createTexture2DArray(UIImageNames, "textures-sym");
	GLuint textureArrayNumeric = render::createTexture2DArray(symbolNames, "textures-sym");
	GLuint skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);


	GLuint visplaneUBO = render::createVisplaneUBO();
	GLuint wallUBO = render::createWallUBO();
	GLuint spriteUBO = render::createSpriteUBO();
	GLuint lightUBO = render::createLightUBO();
	GLuint textObjectUBO = render::createTextObjectUBO();


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
	bool interactKey = false, shouldTakeScreenshot = false;
	int lightFlickerRNG, freq = 0;

	while (!glfwWindowShouldClose(Window)) {
		tick++;
		double frameStart = glfwGetTime();
		glfwPollEvents();

		// Get inputs for this frame
		for (auto &pair : userBindings) {
			std::string functionName = pair.first;
			int keyEnum = pair.second;
			if (keyEnum == -1) {
				raise(functionName + " was not bound to a key!");
			}

			int keyState = glfwGetKey(Window, keyEnum);
			if (keyState == GLFW_PRESS) {
				//Handle specific on-press type use-cases.
				if (functionName == "USE_HEADLAMP" && !keyMap["USE_HEADLAMP"]) {
					headLampEnabled = !headLampEnabled;
				}
				interactKey = (functionName == "USE_INTERACT") && (!keyMap["USE_INTERACT"]);
				shouldTakeScreenshot = (functionName == "META_SCREENSHOT") && (!keyMap["META_SCREENSHOT"]);

				keyMap[functionName] = true;


			} else if (keyState == GLFW_RELEASE) {
				keyMap[functionName] = false;
			}
		}


		//Meta controls
		if (keyMap["META_EXIT"]) {
			break; //Quit
		}

		if (keyMap["META_FREECURSOR"]) {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);			
		} else {
			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
		}

		if (keyMap["META_RELOAD_STAGE"]) {
			loader::loadStage(
				userConfig["META_STAGE_NAME"], &player,
				&visplaneData, &wallData,
				&spriteData, &lightData,
				&textObjectData,
				&logicGates, &flags,
				&textureNames
			);
		} else if (keyMap["META_RELOAD_ENV"]) {
			utils::Player tmpPlayer;
			loader::loadStage(
				userConfig["META_STAGE_NAME"], &tmpPlayer,
				&visplaneData, &wallData,
				&spriteData, &lightData,
				&textObjectData,
				&logicGates, &flags,
				&textureNames
			);
		}


		//Crouch changes physical height
		if (keyMap["MOVE_CROUCH"]) {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_CROUCH;
			player.touchingFloor = false;
		} else {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_STAND;
		}



		float rayAngle = (keyMap["USE_VIEWZOOM"]) ? utils::configToFloat("VIEW_FOV")/(display::ZOOM_MULT * 2.0f) : utils::configToFloat("VIEW_FOV")/2.0f;

		double cursorXDelta = cursorXPos - cursorXPosPrev;
		double cursorYDelta = cursorYPos - cursorYPosPrev;
		player.viewAngle += cursorXDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / display::ZOOM_MULT);
		player.viewAngle = utils::angleClamp(player.viewAngle);
		if (utils::configToBool("VIEW_VLOOK")) {
			double dY = cursorYDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / display::ZOOM_MULT);
			player.vLook = glm::clamp(float(player.vLook+dY), -22.5f, 22.5f);
		}


		//Update logic states.
		for (int index=0; index<constants::MAX_GATES; index++) {
			LogicGate gate = logicGates[index];
			if (gate.gateType == G_INVALID) {continue;}
			gate.evaluateState();
			logicGates[index] = gate;
		}
		physics::updateSpecials(&wallData, &visplaneData, &player, freq, interactKey);


		physics::playerMove(&player, freq, &wallData, &spriteData, &visplaneData);
		float viewBob = (utils::configToBool("VIEW_BOB")) ? render::viewBob(tick, player) : 0.0f;
		player.cameraPosition = player.position + glm::vec3(0.0f, 0.0f, (player.height/3.0f) + viewBob);



		glm::vec4 tintData = render::manageScreenTint(0, player.state);



		//Update Dynamic UBOs.
		render::updateVisplaneUBO(visplaneUBO, &visplaneData);
		render::updateWallUBO(wallUBO, &wallData);
		render::updateSpriteUBO(spriteUBO, &spriteData);
		render::updateLightUBO(lightUBO, &lightData);
		render::updateTextObjectUBO(textObjectUBO, &textObjectData, &symbolNames);
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


		//Camera Data
		GLuint maxVDistLocation = glGetUniformLocation(envShader, "maxRayDistance");
		GLuint maxRAngleLocation = glGetUniformLocation(envShader, "maxRayAngle");
		GLuint zoomFactorLocation = glGetUniformLocation(envShader, "zoomFactor");
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
		glUniform1f(zoomFactorLocation, display::ZOOM_MULT);

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
		glUniform1i(zoomLocation, keyMap["USE_VIEWZOOM"]);

		//Debug
		GLuint uvLocation = glGetUniformLocation(envShader, "drawUV");
		glUniform1i(uvLocation, utils::configToIntBool("META_DRAW_UV"));

		//Headlamp
		GLuint lightLocation = glGetUniformLocation(envShader, "headLampEnabled");
		GLuint lightFlickerLocation = glGetUniformLocation(envShader, "headLampFlicker");
		glUniform1i(lightLocation, headLampEnabled);
		glUniform1i(lightFlickerLocation, lightFlickerRNG);

		//Sun
		GLuint sunDirLocation = glGetUniformLocation(envShader, "sunDirection");
		GLuint sunColourLocation = glGetUniformLocation(envShader, "sunColour");
		glUniform3f(sunDirLocation, stageData.sunDirection.x, stageData.sunDirection.y, stageData.sunDirection.z);
		glUniform3f(sunColourLocation, stageData.sunColour.x, stageData.sunColour.y, stageData.sunColour.z);


		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Environment Shader", true);



		//Sprite Shader.
		glUseProgram(spriteShader);
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindTextureUnit(0, textureArrayEnvironment);


		//Camera Data
		maxVDistLocation = glGetUniformLocation(spriteShader, "maxRayDistance");
		maxRAngleLocation = glGetUniformLocation(spriteShader, "maxRayAngle");
		zoomFactorLocation = glGetUniformLocation(spriteShader, "zoomFactor");
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
		glUniform1f(zoomFactorLocation, display::ZOOM_MULT);

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
		glUniform1i(zoomLocation, keyMap["USE_VIEWZOOM"]);

		//Debug
		uvLocation = glGetUniformLocation(spriteShader, "drawUV");
		glUniform1i(uvLocation, utils::configToIntBool("META_DRAW_UV"));

		//Headlamp
		lightLocation = glGetUniformLocation(spriteShader, "headLampEnabled");
		lightFlickerLocation = glGetUniformLocation(spriteShader, "headLampFlicker");
		glUniform1i(lightLocation, headLampEnabled);
		glUniform1i(lightFlickerLocation, lightFlickerRNG);

		//Sun
		sunDirLocation = glGetUniformLocation(spriteShader, "sunDirection");
		sunColourLocation = glGetUniformLocation(spriteShader, "sunColour");
		glUniform3f(sunDirLocation, stageData.sunDirection.x, stageData.sunDirection.y, stageData.sunDirection.z);
		glUniform3f(sunColourLocation, stageData.sunColour.x, stageData.sunColour.y, stageData.sunColour.z);


		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Sprite Shader", true);

		
		if (!(utils::configToBool("META_SCREENSHOT_HAS_HUD")) && shouldTakeScreenshot) {
			render::saveScreenshot(renderedFrameID);
		}

		//UI Shader.
		if (utils::configToBool("VIEW_SHOW_HUD")) {
			glUseProgram(uiShader);
			glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

			glBindTextureUnit(0, textureArrayEnvironment); //World Textures
			glBindTextureUnit(1, textureArrayUI); //UI Textures
			glBindTextureUnit(2, textureArrayNumeric); //0-9 Textures.



			//Camera Data
			maxVDistLocation = glGetUniformLocation(uiShader, "maxRayDistance");
			maxRAngleLocation = glGetUniformLocation(uiShader, "maxRayAngle");
			zoomFactorLocation = glGetUniformLocation(uiShader, "zoomFactor");
			glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
			glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
			glUniform1f(zoomFactorLocation, display::ZOOM_MULT);

			//Player Data
			playerPosLocation = glGetUniformLocation(uiShader, "playerPosition");
			playerAngleLocation = glGetUniformLocation(uiShader, "playerViewAngle");
			playerRollLocation = glGetUniformLocation(uiShader, "playerViewRoll");
			playerPitchLocation = glGetUniformLocation(uiShader, "playerViewPitch");
			zoomLocation = glGetUniformLocation(uiShader, "zoom");
			glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
			glUniform1f(playerAngleLocation, player.viewAngle);
			glUniform1f(playerRollLocation, player.viewRoll);
			glUniform1f(playerPitchLocation, player.viewPitch);
			glUniform1i(zoomLocation, keyMap["USE_VIEWZOOM"]);

			//Player Data.
			GLuint vignetteColourLocation = glGetUniformLocation(uiShader, "screenTint");
			GLuint healthLocation = glGetUniformLocation(uiShader, "health");
			GLuint energyLocation = glGetUniformLocation(uiShader, "energy");
			glUniform4f(vignetteColourLocation, tintData.x, tintData.y, tintData.z, tintData.w);
			glUniform1i(healthLocation, player.health);
			glUniform1i(energyLocation, player.energy);

			//Assorted other data.
			GLuint screenResLocation = glGetUniformLocation(uiShader, "screenResolution");
			GLuint freqLocation = glGetUniformLocation(uiShader, "freq");
			GLuint showFreqLocation = glGetUniformLocation(uiShader, "showFreq");
			glUniform2i(screenResLocation, currentScreenRes.x, currentScreenRes.y);
			glUniform1i(freqLocation, freq);
			glUniform1i(showFreqLocation, utils::configToIntBool("META_SHOW_FREQ_UI"));


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
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindTextureUnit(0, renderedFrameID);


		//Assorted other data.
		maxVDistLocation = glGetUniformLocation(displayShader, "maxRayDistance");
		GLuint screenResLocation = glGetUniformLocation(displayShader, "screenResolution");
		GLuint antiAliasLocation = glGetUniformLocation(displayShader, "antiAliasingLevel");
		GLuint smoothingLocation = glGetUniformLocation(displayShader, "smoothingEnabled");
		GLuint quantLocation = glGetUniformLocation(displayShader, "quantisingLevel");
		GLuint screenHUDLocation = glGetUniformLocation(displayShader, "screenshotHasHUD");
		glUniform2i(screenResLocation, currentScreenRes.x, currentScreenRes.y);
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1i(antiAliasLocation, utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
		glUniform1i(smoothingLocation, utils::configToBool("VIEW_SMOOTHING"));
		glUniform1i(quantLocation, utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));
		glUniform1i(screenHUDLocation, utils::configToBool("META_SCREENSHOT_HAS_HUD"));



		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Display Shader", true);


		if (utils::configToBool("META_SCREENSHOT_HAS_HUD") && shouldTakeScreenshot) {
			render::saveScreenshot(renderedFrameID);
		}



		while (glfwGetTime() - frameStart < (1.0f/utils::configToFloat("VIEW_MAX_FREQ"))) {}
		double totalTime = (glfwGetTime() - frameStart);
		freq = floor(1/totalTime);
		if (utils::configToBool("META_SHOW_FREQ_CONSOLE")) {
			std::cout << freq << std::endl;
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
