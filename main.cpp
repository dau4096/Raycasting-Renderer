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
	//Update SSBOs for shadowmapping.
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
	utils::GLErrorcheck("Updating SSBOs [Initialisation]", true);




	//Create textures and arrays;
	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLuint interfaceID = render::createGLImage2D(display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
	GLuint textureArrayEnvironment = render::createTexture2DArray(textureNames, "textures-env", true);
	GLuint textureArrayUI = render::createTexture2DArray(UIImageNames, "textures-sym");
	GLuint textureArrayNumeric = render::createTexture2DArray(symbolNames, "textures-sym");
	GLuint skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);
	GLuint shadowMapsID = render::createShadowMaps(render::bindCommonUniforms);


	//Environment shader
	GLuint envShader = render::createShaderProgram("environment", false, false);

	//Sprite Shader
	GLuint spriteShader = render::createShaderProgram("sprites", false, false);

	//uiShader
	GLuint uiShader = render::createShaderProgram("interface", false, false);

	//Display Shader
	GLuint displayShader = render::createShaderProgram("display", false, true);



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
		zoomEffect = ((keyMap["USE_VIEWZOOM"]) ? display::ZOOM_MULT : 1.0f);
		double cursorXDelta = cursorXPos - cursorXPosPrev;
		double cursorYDelta = cursorYPos - cursorYPosPrev;
		player.viewAngle += cursorXDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / zoomEffect);
		player.viewAngle = fmodf(player.viewAngle + 540.0f, 360.0f) - 180.0f;
		if (utils::configToBool("VIEW_VLOOK")) {
			double dY = cursorYDelta * (utils::configToFloat("TURN_SPEED_MOUSE") / zoomEffect);
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
		utils::GLErrorcheck("Updating SSBOs [Running]", true);



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
		glBindTextureUnit(2, shadowMapsID);

		//Uniforms
		render::bindCommonUniforms(envShader, &player);
		//Other
		render::bindUniformValue(envShader, "headLampEnabled", headLampEnabled);
		render::bindUniformValue(envShader, "headLampFlicker", lightFlickerRNG);
		render::bindUniformValue(envShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		utils::GLErrorcheck("Environment Shader", true);



		//Sprite Shader.
		glUseProgram(spriteShader);
		glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindTextureUnit(0, textureArrayEnvironment);

		//Uniforms
		render::bindCommonUniforms(spriteShader, &player);
		//Other
		render::bindUniformValue(spriteShader, "headLampEnabled", headLampEnabled);
		render::bindUniformValue(spriteShader, "headLampFlicker", lightFlickerRNG);

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

			//Uniforms
			render::bindCommonUniforms(uiShader, &player);
			//UI-Specific
			render::bindUniformValue(uiShader, "showFreq", utils::configToBool("META_SHOW_FREQ_UI"));
			render::bindUniformValue(uiShader, "showData", utils::configToBool("META_SHOW_DATA"));
			render::bindUniformValue(uiShader, "health", player.health);
			render::bindUniformValue(uiShader, "energy", player.energy);
			render::bindUniformValue(uiShader, "freq", freq);

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

		//Uniforms
		render::bindCommonUniforms(displayShader, &player);
		//Display specific data.
		render::bindUniformValue(displayShader, "antiAliasingLevel", utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
		render::bindUniformValue(displayShader, "smoothingEnabled", utils::configToBool("VIEW_SMOOTHING"));
		render::bindUniformValue(displayShader, "quantisingLevel", utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));

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
