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
	"symbol_DASH", "symbol_DOT",
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



bool headLampEnabled = false;
GLuint renderedFrameID;


void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentWindowResolution = glm::ivec2(width, height);
	currentRenderResolution = glm::ivec2(
		glm::min(width, desiredRenderResolution.x),
		glm::min(height, desiredRenderResolution.y)
	);

	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	verticalFOV = 2 * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (currentRenderResolution.x / currentRenderResolution.y));
}





int main() {
	try { //Catch exceptions
	utils::Player player;
	std::vector<utils::Visplane> visplaneData;
	std::vector<utils::Wall> wallData;
	std::vector<utils::Displacement> displacementData;
	std::vector<utils::Sprite> spriteData;
	std::vector<utils::Light> lightData;
	std::vector<utils::TextObject> textObjectData;
	std::vector<utils::LogicGate> logicGates;
	std::array<int, constants::MAX_FLAGS> flags;

	std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;

	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		&visplaneData, &wallData, &displacementData,
		&spriteData, &lightData,
		&textObjectData,
		&logicGates, &flags,
		&textureNames
	);


	double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
	currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;
	currentRenderResolution = glm::ivec2(
		glm::min(display::INITIAL_SCREEN_RESOLUTION.x, desiredRenderResolution.x),
		glm::min(display::INITIAL_SCREEN_RESOLUTION.y, desiredRenderResolution.y)
	);


	GLFWwindow* Window = render::initializeWindow(currentWindowResolution.x, currentWindowResolution.y, "Raycasting-Renderer/GPU");
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	glEnable(GL_BLEND);

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);




	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLuint interfaceID = render::createGLImage2D(display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
	GLuint textureArrayEnvironment = render::createTexture2DArray(textureNames);
	GLuint textureArrayUI = render::createTexture2DArray(UIImageNames, "textures-sym");
	GLuint textureArrayNumeric = render::createTexture2DArray(symbolNames, "textures-sym");
	GLuint skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);


	GLuint visplaneSSBO = render::createShaderStorageBufferObject(
		0, sizeof(utils::VisplaneGPU) * validVisplanes
	);
	GLuint wallSSBO = render::createShaderStorageBufferObject(
		1, sizeof(utils::WallGPU) * validWalls
	);
	GLuint spriteSSBO = render::createShaderStorageBufferObject(
		2, sizeof(utils::SpriteGPU) * validSprites
	);
	GLuint lightSSBO = render::createShaderStorageBufferObject(
		3, sizeof(utils::LightGPU) * validLights
	);
	GLuint textObjectSSBO = render::createShaderStorageBufferObject(
		4, sizeof(utils::TextObjectGPU) * validTextObjects
	);
	GLuint displacementSSBO = render::createShaderStorageBufferObject(
		5, sizeof(utils::DisplacementGPU) * validDisplacements
	);


	//Environment shader
	GLuint envShader = render::createShaderProgram("environment", false);

	//Sprite Shader
	GLuint spriteShader = render::createShaderProgram("sprites", false);

	//uiShader
	GLuint uiShader = render::createShaderProgram("interface", false);

	//Display Shader
	GLuint displayShader = render::createShaderProgram("display");



	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glDisable(GL_DEPTH_TEST);
	GLuint VAO = render::getVAO();

	verticalFOV = 2 * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (currentRenderResolution.x / currentRenderResolution.y));


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
				&visplaneData, &wallData, &displacementData,
				&spriteData, &lightData,
				&textObjectData,
				&logicGates, &flags,
				&textureNames
			);
			if (utils::configToBool("META_DYNAMIC_UPD_ALLOW_NEW_TEXTURES")) {
				textureArrayEnvironment = render::createTexture2DArray(textureNames);
				skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);
			}
		} else if (keyMap["META_RELOAD_ENV"] || utils::configToBool("META_DYNAMIC_UPD")) {
			utils::Player tmpPlayer;
			loader::loadStage(
				userConfig["META_STAGE_NAME"], &tmpPlayer,
				&visplaneData, &wallData, &displacementData,
				&spriteData, &lightData,
				&textObjectData,
				&logicGates, &flags,
				&textureNames
			);
			if (utils::configToBool("META_DYNAMIC_UPD_ALLOW_NEW_TEXTURES")) {
				textureArrayEnvironment = render::createTexture2DArray(textureNames);
				skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);
			}
		}


		//Crouch changes physical height
		if (keyMap["MOVE_CROUCH"]) {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_CROUCH;
			player.touchingFloor = false;
		} else {
			player.height = playerConfig::PLAYER_COLLISION_HEIGHT_STAND;
		}



		rayAngle = (keyMap["USE_VIEWZOOM"]) ? utils::configToFloat("VIEW_FOV")/(display::ZOOM_MULT * 2.0f) : utils::configToFloat("VIEW_FOV")/2.0f;

		double cursorXDelta = cursorXPos - cursorXPosPrev;
		double cursorYDelta = cursorYPos - cursorYPosPrev;
		player.viewAngle += cursorXDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / display::ZOOM_MULT);
		player.viewAngle = fmodf(player.viewAngle + 540.0f, 360.0f) - 180.0f;
		if (utils::configToBool("VIEW_VLOOK")) {
			double dY = cursorYDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / display::ZOOM_MULT);
			player.vLook = glm::clamp(float(player.vLook+dY), -22.5f, 22.5f);
		}


		//Update logic states.
		for (int index=0; index<validGates; index++) {
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



		//Update SSBOs.
		render::updateShaderStorageBufferObject<utils::VisplaneGPU>(
			visplaneSSBO, &player, &visplaneData
		);
		render::updateShaderStorageBufferObject<utils::WallGPU>(
			wallSSBO, &player, &wallData
		);
		render::updateShaderStorageBufferObject<utils::DisplacementGPU>(
			displacementSSBO, &player, &displacementData
		);
		render::updateShaderStorageBufferObject<utils::SpriteGPU>(
			spriteSSBO, &player, &spriteData
		);
		render::updateShaderStorageBufferObject<utils::LightGPU>(
			lightSSBO, &player, &lightData
		);
		render::updateShaderStorageBufferObject<utils::TextObjectGPU>(
			textObjectSSBO, &player, &textObjectData, &symbolNames
		);
		utils::GLErrorcheck("Updating SSBOs", true);



		if (headLampEnabled) {
			lightFlickerRNG = utils::RNGc();
		}



		//Update resolution
		glViewport(0, 0, currentRenderResolution.x, currentRenderResolution.y);


		//Environment Shader.
		glUseProgram(envShader);
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glBindTextureUnit(0, textureArrayEnvironment);
		glBindTextureUnit(1, skyboxTextureID);


		//Camera Data
		GLuint maxVDistLocation = glGetUniformLocation(envShader, "maxRayDistance");
		GLuint maxRAngleLocation = glGetUniformLocation(envShader, "maxRayAngle");
		GLuint vFOVLocation = glGetUniformLocation(envShader, "verticalFOV");
		GLuint zoomFactorLocation = glGetUniformLocation(envShader, "zoomFactor");
		GLuint texScaleLocation = glGetUniformLocation(envShader, "textureScale");
		GLuint texOffsetLocation = glGetUniformLocation(envShader, "textureOffset");
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
		glUniform1f(vFOVLocation, verticalFOV);
		glUniform1f(zoomFactorLocation, display::ZOOM_MULT);
		glUniform2f(texScaleLocation, stageData.textureScale.x, stageData.textureScale.y);
		glUniform3f(texOffsetLocation, stageData.textureOffset.x, stageData.textureOffset.y, stageData.textureOffset.z);

		//Player Data
		GLuint playerPosLocation = glGetUniformLocation(envShader, "playerPosition");
		GLuint playerAngleLocation = glGetUniformLocation(envShader, "playerViewAngle");
		GLuint playerRollLocation = glGetUniformLocation(envShader, "playerViewRoll");
		GLuint playerPitchLocation = glGetUniformLocation(envShader, "playerViewPitch");
		GLuint zoomLocation = glGetUniformLocation(envShader, "zoom");
		GLuint renderResLocation = glGetUniformLocation(envShader, "renderResolution");
		glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1f(playerRollLocation, player.viewRoll);
		glUniform1f(playerPitchLocation, player.viewPitch);
		glUniform1i(zoomLocation, keyMap["USE_VIEWZOOM"]);
		glUniform2i(renderResLocation, currentRenderResolution.x, currentRenderResolution.y);

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

		//Other
		GLuint numVisplanesLocation = glGetUniformLocation(envShader, "numVisplanes");
		GLuint numWallsLocation = glGetUniformLocation(envShader, "numWalls");
		GLuint numDispsLocation = glGetUniformLocation(envShader, "numDisplacements");
		GLuint numLightsLocation = glGetUniformLocation(envShader, "numLights");
		glUniform1i(numVisplanesLocation, validVisplanes);
		glUniform1i(numWallsLocation, validWalls);
		glUniform1i(numDispsLocation, validDisplacements);
		glUniform1i(numLightsLocation, validLights);


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
		vFOVLocation = glGetUniformLocation(spriteShader, "verticalFOV");
		zoomFactorLocation = glGetUniformLocation(spriteShader, "zoomFactor");
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
		glUniform1f(vFOVLocation, verticalFOV);
		glUniform1f(zoomFactorLocation, display::ZOOM_MULT);

		//Player Data
		playerPosLocation = glGetUniformLocation(spriteShader, "playerPosition");
		playerAngleLocation = glGetUniformLocation(spriteShader, "playerViewAngle");
		playerRollLocation = glGetUniformLocation(spriteShader, "playerViewRoll");
		playerPitchLocation = glGetUniformLocation(spriteShader, "playerViewPitch");
		zoomLocation = glGetUniformLocation(spriteShader, "zoom");
		renderResLocation = glGetUniformLocation(spriteShader, "renderResolution");
		glUniform3f(playerPosLocation, player.cameraPosition.x, player.cameraPosition.y, player.cameraPosition.z);
		glUniform1f(playerAngleLocation, player.viewAngle);
		glUniform1f(playerRollLocation, player.viewRoll);
		glUniform1f(playerPitchLocation, player.viewPitch);
		glUniform1i(zoomLocation, keyMap["USE_VIEWZOOM"]);
		glUniform2i(renderResLocation, currentRenderResolution.x, currentRenderResolution.y);

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

		//Other
		numVisplanesLocation = glGetUniformLocation(spriteShader, "numVisplanes");
		numWallsLocation = glGetUniformLocation(spriteShader, "numWalls");
		GLuint numSpritesLocation = glGetUniformLocation(spriteShader, "numSprites");
		numLightsLocation = glGetUniformLocation(spriteShader, "numLights");
		glUniform1i(numVisplanesLocation, validVisplanes);
		glUniform1i(numWallsLocation, validWalls);
		glUniform1i(numSpritesLocation, validSprites);
		glUniform1i(numLightsLocation, validLights);


		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Sprite Shader", true);


		//UI Shader.
		if (utils::configToBool("VIEW_SHOW_HUD")) {
			glViewport(0, 0, display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
			glUseProgram(uiShader);
			glBindTextureUnit(0, renderedFrameID);
			glBindImageTexture(0, interfaceID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

			glBindTextureUnit(1, textureArrayEnvironment); //World Textures
			glBindTextureUnit(2, textureArrayUI); //UI Textures
			glBindTextureUnit(3, textureArrayNumeric); //0-9 Textures.



			//Camera Data
			maxVDistLocation = glGetUniformLocation(uiShader, "maxRayDistance");
			maxRAngleLocation = glGetUniformLocation(uiShader, "maxRayAngle");
			zoomFactorLocation = glGetUniformLocation(uiShader, "zoomFactor");
			GLuint interfaceResLocation = glGetUniformLocation(uiShader, "interfaceResolution");
			renderResLocation = glGetUniformLocation(uiShader, "renderResolution");
			glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
			glUniform1f(maxRAngleLocation, utils::configToFloat("VIEW_FOV") / 2.0f);
			glUniform1f(zoomFactorLocation, display::ZOOM_MULT);
			glUniform2i(interfaceResLocation, display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
			glUniform2i(renderResLocation, currentRenderResolution.x, currentRenderResolution.y);

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
			GLuint showDataLocation = glGetUniformLocation(uiShader, "showData");
			GLuint numTextObjectsLocation = glGetUniformLocation(uiShader, "numTextObjects");
			glUniform2i(screenResLocation, currentWindowResolution.x, currentWindowResolution.y);
			glUniform1i(freqLocation, freq);
			glUniform1i(showFreqLocation, utils::configToBool("META_SHOW_FREQ_UI"));
			glUniform1i(showDataLocation, utils::configToBool("META_SHOW_DATA"));
			glUniform1i(numTextObjectsLocation, validVisplanes);


			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			utils::GLErrorcheck("UI Shader", true);
		}		


		//Update resolution
		glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);

		//Display Shader and update screen.
		glUseProgram(displayShader);
		glBindTextureUnit(0, renderedFrameID);
		glBindTextureUnit(1, interfaceID);


		//Assorted other data.
		maxVDistLocation = glGetUniformLocation(displayShader, "maxRayDistance");
		GLuint screenResLocation = glGetUniformLocation(displayShader, "screenResolution");
		renderResLocation = glGetUniformLocation(displayShader, "renderResolution");
		GLuint antiAliasLocation = glGetUniformLocation(displayShader, "antiAliasingLevel");
		GLuint smoothingLocation = glGetUniformLocation(displayShader, "smoothingEnabled");
		GLuint quantLocation = glGetUniformLocation(displayShader, "quantisingLevel");
		glUniform2i(screenResLocation, currentWindowResolution.x, currentWindowResolution.y);
		glUniform2i(renderResLocation, currentRenderResolution.x, currentRenderResolution.y);
		glUniform1f(maxVDistLocation, utils::configToFloat("VIEW_MAX_RAY_DIST"));
		glUniform1i(antiAliasLocation, utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
		glUniform1i(smoothingLocation, utils::configToBool("VIEW_SMOOTHING"));
		glUniform1i(quantLocation, utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));



		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glfwSwapBuffers(Window);
		utils::GLErrorcheck("Display Shader", true);


		if (shouldTakeScreenshot) {
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
