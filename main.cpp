#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma execution_character_set("utf-8")

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




GLFWwindow* Window;
utils::Player player;
std::atomic<bool> runPhysics = true;
bool headLampEnabled = false;
bool interactKey = false, shouldTakeScreenshot = false;
int lightFlickerRNG;
GLuint renderedFrameID, interfaceID, positionMapID, normalMapID, shadowMapID;
GLuint envShader, spriteShader, lightingShader, uiShader, displayShader; //Shaders
GLuint textureArrayEnvironment, skyboxTextureID, textureArrayUI, textureArrayNumeric; //Textures
GLuint visplaneSSBO, wallSSBO, spriteSSBO, lightSSBO, textObjectSSBO, displacementSSBO; //Storage Buffers
GLuint VAO;



void framebufferSizeCallback(GLFWwindow* Window, int width, int height) {
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentWindowResolution = glm::ivec2(width, height);
	currentRenderResolution = glm::ivec2(
		glm::min(width, desiredRenderResolution.x),
		glm::min(height, desiredRenderResolution.y)
	);
	currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));

	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	positionMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	normalMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	shadowMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y, GL_RGBA32F, GL_LINEAR);
	verticalFOV = 2 * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
}



void APIENTRY openGLErrorCallback(
		GLenum source,
		GLenum type, GLuint id,
		GLenum severity,
		GLsizei length, const GLchar* message,
		const void* userParam
	) {
	/*
	Nicely formatted callback from;
	[https://learnopengl.com/In-Practice/Debugging]
	*/
	if(id == 131169 || id == 131185 || id == 131218 || id == 131204) {return;}

	std::cout << "---------------" << std::endl << "Debug message (" << id << ") | " << message << std::endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             {std::cout << "Source: API"; break;}
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   {std::cout << "Source: Window System"; break;}
		case GL_DEBUG_SOURCE_SHADER_COMPILER: {std::cout << "Source: Shader Compiler"; break;}
		case GL_DEBUG_SOURCE_THIRD_PARTY:     {std::cout << "Source: Third Party"; break;}
		case GL_DEBUG_SOURCE_APPLICATION:     {std::cout << "Source: Application"; break;}
		case GL_DEBUG_SOURCE_OTHER:           {std::cout << "Source: Other"; break;}
	} std::cout << std::endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               {std::cout << "Type: Error"; break;}
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {std::cout << "Type: Deprecated Behaviour"; break;}
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  {std::cout << "Type: Undefined Behaviour"; break;} 
		case GL_DEBUG_TYPE_PORTABILITY:         {std::cout << "Type: Portability"; break;}
		case GL_DEBUG_TYPE_PERFORMANCE:         {std::cout << "Type: Performance"; break;}
		case GL_DEBUG_TYPE_MARKER:              {std::cout << "Type: Marker"; break;}
		case GL_DEBUG_TYPE_PUSH_GROUP:          {std::cout << "Type: Push Group"; break;}
		case GL_DEBUG_TYPE_POP_GROUP:           {std::cout << "Type: Pop Group"; break;}
		case GL_DEBUG_TYPE_OTHER:               {std::cout << "Type: Other"; break;}
	} std::cout << std::endl;
	
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         {std::cout << "Severity: high"; break;}
		case GL_DEBUG_SEVERITY_MEDIUM:       {std::cout << "Severity: medium"; break;}
		case GL_DEBUG_SEVERITY_LOW:          {std::cout << "Severity: low"; break;}
		case GL_DEBUG_SEVERITY_NOTIFICATION: {std::cout << "Severity: notification"; break;}
	} std::cout << std::endl;
	std::cout << std::endl;

	utils::pause();
}



std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;
void prepareOpenGL() {
	//OpenGL setup;

	//Image2Ds
	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	interfaceID = render::createGLImage2D(display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
	positionMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	normalMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	shadowMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y, GL_RGBA32F, GL_LINEAR);

	//Textures
	textureArrayEnvironment = render::createTexture2DArray(textureNames, "textures-env", true);
	textureArrayUI = render::createTexture2DArray(UIImageNames, "textures-sym");
	textureArrayNumeric = render::createTexture2DArray(symbolNames, "textures-sym");
	skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);


	visplaneSSBO = render::createShaderStorageBufferObject(
		0, sizeof(utils::VisplaneGPU) * validVisplanes
	);
	wallSSBO = render::createShaderStorageBufferObject(
		1, sizeof(utils::WallGPU) * validWalls
	);
	spriteSSBO = render::createShaderStorageBufferObject(
		2, sizeof(utils::SpriteGPU) * validSprites
	);
	lightSSBO = render::createShaderStorageBufferObject(
		3, sizeof(utils::LightGPU) * validLights
	);
	textObjectSSBO = render::createShaderStorageBufferObject(
		4, sizeof(utils::TextObjectGPU) * validTextObjects
	);
	displacementSSBO = render::createShaderStorageBufferObject(
		5, sizeof(utils::DisplacementGPU) * validDisplacements
	);


	//Environment shader
	envShader = render::createShaderProgram("environment", false);

	//Sprite Shader
	spriteShader = render::createShaderProgram("sprites", false);

	//Shadow Shader
	lightingShader = render::createShaderProgram("lighting", false);

	//uiShader
	uiShader = render::createShaderProgram("interface", false);

	//Display Shader
	displayShader = render::createShaderProgram("display");



	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glDisable(GL_DEPTH_TEST);
	VAO = render::getVAO();

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
	

	//Debug settings
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openGLErrorCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	utils::GLErrorcheck("Initialisation", true); //Old basic debugging
}



inline void renderingGeneric(const std::string& shaderName="") {
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	if (!shaderName.empty()) {
		utils::GLErrorcheck(shaderName, true);
	}
}

void renderFrame(double blendingAlpha) {
	//Update resolution
	glViewport(0, 0, currentRenderResolution.x, currentRenderResolution.y);
	if (headLampEnabled) {
		lightFlickerRNG = utils::RNGc();
	}
	glm::vec4 tintData = render::manageScreenTint(0, player.state);
	float viewBob = (utils::configToBool("VIEW_BOB")) ? render::viewBob(tick, player) : 0.0f;
	glm::vec3 interpPosition = glm::mix(player.prevPosition, player.position, blendingAlpha);
	player.cameraPosition = interpPosition + glm::vec3(0.0f, 0.0f, (player.height/3.0f) + viewBob);



	//Environment Shader.
	glUseProgram(envShader);
	glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, textureArrayEnvironment);
	glBindTextureUnit(1, skyboxTextureID);

	//Uniforms
	render::bindCommonUniforms(envShader, &player);
	//Other
	render::bindUniformValue(envShader, "headLampEnabled", headLampEnabled);
	render::bindUniformValue(envShader, "headLampFlicker", lightFlickerRNG);
	render::bindUniformValue(envShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));

	renderingGeneric("Environment Shader");



	//Sprite Shader.
	glUseProgram(spriteShader);
	glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, textureArrayEnvironment);

	//Uniforms
	render::bindCommonUniforms(spriteShader, &player);
	//Other
	render::bindUniformValue(spriteShader, "headLampEnabled", headLampEnabled);
	render::bindUniformValue(spriteShader, "headLampFlicker", lightFlickerRNG);

	renderingGeneric("Sprite Shader");



	//Shadow Shader
	glViewport(0, 0, currentShadowResolution.x, currentShadowResolution.y);
	glUseProgram(lightingShader);

	glBindTextureUnit(0, positionMapID);
	glBindTextureUnit(1, normalMapID);
	glBindImageTexture(0, shadowMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//Uniforms;
	render::bindCommonUniforms(lightingShader, &player);

	renderingGeneric("Shadow Shader");



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
		render::bindUniformValue(uiShader, "showFramerate", utils::configToBool("META_SHOW_FRAMERATE_UI"));
		render::bindUniformValue(uiShader, "framerate", int(round(framerate)));
		render::bindUniformValue(uiShader, "showTickrate", utils::configToBool("META_SHOW_TICKRATE_UI"));
		render::bindUniformValue(uiShader, "tickrate", int(round(tickrate)));
		render::bindUniformValue(uiShader, "showData", utils::configToBool("META_SHOW_DATA"));
		render::bindUniformValue(uiShader, "health", player.health);
		render::bindUniformValue(uiShader, "energy", player.energy);
		render::bindUniformValue(uiShader, "tint", tintData);

		renderingGeneric("UI Shader");
	}		


	//Display Shader and update screen.
	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glUseProgram(displayShader);

	glBindTextureUnit(0, renderedFrameID);
	glBindTextureUnit(1, interfaceID);
	glBindTextureUnit(2, shadowMapID);
	glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//Uniforms
	render::bindCommonUniforms(displayShader, &player);
	//Display specific data.
	render::bindUniformValue(displayShader, "antiAliasingLevel", utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
	render::bindUniformValue(displayShader, "smoothingEnabled", utils::configToBool("VIEW_SMOOTHING"));
	render::bindUniformValue(displayShader, "quantisingLevel", utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));
	render::bindUniformValue(displayShader, "screenshotHasHUD", utils::configToBool("VIEW_INTERFACE_IN_SCREENSHOT"));

	renderingGeneric("Display Shader");


	if (shouldTakeScreenshot) {
		render::saveScreenshot(renderedFrameID);
	}
}



//Data must be synced between updateSSBOs() and the physics thread.
utils::DataSet stateA, stateB;
utils::DataSet* physicsData = &stateA;
utils::DataSet* graphicsData = &stateB;
std::mutex stateSwapMutex;

//Non-synced data.
std::vector<utils::LogicGate> logicGates;
std::array<int, constants::MAX_FLAGS> flags;


void updateSSBOs(utils::DataSet* localGraphicsData) {
	//Update SSBOs.
	render::updateShaderStorageBufferObject<utils::VisplaneGPU>(
		visplaneSSBO, &player, &(localGraphicsData->visplaneData)
	);
	render::updateShaderStorageBufferObject<utils::WallGPU>(
		wallSSBO, &player, &(localGraphicsData->wallData)
	);
	render::updateShaderStorageBufferObject<utils::DisplacementGPU>(
		displacementSSBO, &player, &(localGraphicsData->displacementData)
	);
	render::updateShaderStorageBufferObject<utils::SpriteGPU>(
		spriteSSBO, &player, &(localGraphicsData->spriteData)
	);
	render::updateShaderStorageBufferObject<utils::LightGPU>(
		lightSSBO, &player, &(localGraphicsData->lightData)
	);
	render::updateShaderStorageBufferObject<utils::TextObjectGPU>(
		textObjectSSBO, &player, &(localGraphicsData->textObjectData), &symbolNames
	);
	utils::GLErrorcheck("Updating SSBOs", true);
}


double tickStart;
void physicsLoop(bool* physicsReady) {
	double maxTickTime = 1.0f/constants::PHYSICS_FREQUENCY;

	tick = 0;
	while (runPhysics) {
		tickStart = glfwGetTime();
		*physicsReady = false;
		player.prevPosition = player.position;

		//Update logic states.
		for (int index=0; index<validGates; index++) {
			LogicGate gate = logicGates[index];
			if (gate.gateType == G_INVALID) {continue;}
			gate.evaluateState();
			logicGates[index] = gate;
		}
		physics::updateSpecials(&(physicsData->wallData), &(physicsData->visplaneData), &player, interactKey);

		physics::playerMove(&player, &(physicsData->wallData), &(physicsData->spriteData), &(physicsData->visplaneData));


		//Update states;
		{
			std::lock_guard<std::mutex> lock(stateSwapMutex);
			std::swap(physicsData, graphicsData);
		}

		float dt = glfwGetTime() - tickStart;
		tickrate = floor(1.0f / dt);
		if constexpr (dev::SHOW_PHYSICS_TICKRATE) {
			std::cout << "Tickrate: " << tickrate << "Hz" << std::endl;
		}
		if constexpr (dev::SHOW_PHYSICS_DT) {
			std::cout << "Tick #" << tick << " took " << std::setprecision(6) << (dt * 1e6f) << "µs / Hypothetical tickrate: " << static_cast<int>(1.0f / dt) << endl;
		}

		*physicsReady = true;
		while (glfwGetTime() - tickStart < maxTickTime) {std::this_thread::yield();}
		tick++;
	}
}



double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
inline void reloadLevel(const bool resetPlayer=false) {
	if (resetPlayer) {
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &player,
			&(physicsData->visplaneData), &(physicsData->wallData), &(physicsData->displacementData),
			&(physicsData->spriteData), &(physicsData->lightData),
			&(physicsData->textObjectData),
			&(physicsData->logicGates), &(physicsData->flags),
			&textureNames
		);
	} else {
		utils::Player tmpPlayer;
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &tmpPlayer,
			&(physicsData->visplaneData), &(physicsData->wallData), &(physicsData->displacementData),
			&(physicsData->spriteData), &(physicsData->lightData),
			&(physicsData->textObjectData),
			&(physicsData->logicGates), &(physicsData->flags),
			&textureNames
		);
	}
	if (utils::configToBool("META_DYNAMIC_UPD_ALLOW_NEW_TEXTURES")) {
		glDeleteTextures(1, &textureArrayEnvironment);
		textureArrayEnvironment = render::createTexture2DArray(textureNames);
		skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);
	}
	*graphicsData = *physicsData;
}

void handleInputs() {
	glfwPollEvents();

	//Get inputs for this frame
	for (auto &pair : userBindings) {
		std::string functionName = pair.first;
		int keyEnum = pair.second;
		if (keyEnum == -1) {
			std::cout << functionName  << " was not bound to a key!" << std::endl;;
			continue;
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
	if (keyMap["META_FREECURSOR"]) {
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	} else {
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	}

	if (keyMap["META_RELOAD_STAGE"]) {
		reloadLevel(true);
	} else if (keyMap["META_RELOAD_ENV"] || utils::configToBool("META_DYNAMIC_UPD")) {
		reloadLevel(false);
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
}


std::thread physicsThread;
inline void stopPhysics() {
	runPhysics = false;
	if (physicsThread.joinable()) {
		physicsThread.join();
	}
}

int main() {
	try { //Catch exceptions
	SetConsoleOutputCP(65001); //CP_UTF8

	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		&(physicsData->visplaneData), &(physicsData->wallData), &(physicsData->displacementData),
		&(physicsData->spriteData), &(physicsData->lightData),
		&(physicsData->textObjectData),
		&(physicsData->logicGates), &(physicsData->flags),
		&textureNames
	);


	currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;
	currentRenderResolution = glm::ivec2(
		glm::min(display::INITIAL_SCREEN_RESOLUTION.x, desiredRenderResolution.x),
		glm::min(display::INITIAL_SCREEN_RESOLUTION.y, desiredRenderResolution.y)
	);
	currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));


	Window = render::initializeWindow(currentWindowResolution.x, currentWindowResolution.y, "Raycasting-Renderer/GPU");
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	glEnable(GL_BLEND);
	bool vsync = utils::configToBool("VIEW_VSYNC");
	if (vsync) {
		glfwSwapInterval(1);
	}

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);

	prepareOpenGL();
	*graphicsData = *physicsData;
	updateSSBOs(graphicsData);
	double maxFrameTime = 1.0f/utils::configToFloat("VIEW_MAX_FREQ");


	//Threads;
	bool physicsReady;
	physicsThread = std::thread(physicsLoop, &physicsReady);
	tickStart = glfwGetTime();

	frame = 0;
	while (!glfwWindowShouldClose(Window)) {
		double frameStart = glfwGetTime();
		double blendingAlpha = (frameStart - tickStart) * constants::PHYSICS_FREQUENCY;

		handleInputs();
		if (keyMap["META_EXIT"]) {break; /* Quit Immediately */}



		utils::DataSet* localGraphicsData = nullptr;
		{
			std::lock_guard<std::mutex> lock(stateSwapMutex);
			localGraphicsData = graphicsData;
		}
		updateSSBOs(localGraphicsData);
		renderFrame(blendingAlpha);
		glfwSwapBuffers(Window);


		float dt = glfwGetTime() - frameStart;
		if (utils::configToBool("META_SHOW_DT_CONSOLE")) {
			std::cout << "Frame #" << frame << " took " << std::setprecision(2) << (dt * 1e3f) << "ms / Hypothetical framerate: " << static_cast<int>(1.0f / dt) << endl;
		}
		if (!vsync) {
			while (glfwGetTime() - frameStart < maxFrameTime) {std::this_thread::yield();}
		}
		framerate = floor(1.0f / (glfwGetTime() - frameStart));
		if (utils::configToBool("META_SHOW_FRAMERATE_CONSOLE")) {
			std::cout << "Framerate: " << framerate << "Hz" << std::endl;
		}

		cursorXPosPrev = cursorXPos;
		cursorYPosPrev = cursorYPos;
		frame++;
	}

	//Cleanup OpenGL.
	glDeleteTextures(1, &textureArrayEnvironment);

	stopPhysics();
	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;


	//Catch exceptions.
	} catch (const std::exception& e) {
		stopPhysics();
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An exception was thrown: " << e.what() << std::endl;
		pause();
		return -1;
	} catch (...) {
		stopPhysics();
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An unspecified exception was thrown." << std::endl;
		pause();
		return -1;
	}
}
