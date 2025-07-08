#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma execution_character_set("utf-8")

#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image_write.h"
#include "src/includes.h"
#include "src/global.h"
#include "src/loader.h"
#include "src/physics.h"
#include "src/graphics.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;





GLFWwindow* Window;
utils::Player player;
std::atomic<bool> runPhysics = true;
//Data must be synced between the graphics and physics threads.
utils::DataSet stateA, stateB;
utils::DataSet* physicsData = &stateA;
utils::DataSet* graphicsData = &stateB;
std::mutex stateSwapMutex;



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


	GLIndex::wallIntersectSSBO = graphics::createShaderStorageBufferObject(
		7, sizeof(utils::WallIntersect) * currentRenderResolution.x * validWalls
	);

	GLIndex::renderedFrameID = graphics::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLIndex::positionMapID = graphics::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	GLIndex::normalMapID = graphics::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	GLIndex::lightingMapsArrayID = graphics::createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);
	GLIndex::displacementFBO = graphics::createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);
	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
}







std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;



//Non-synced data.
std::vector<utils::LogicGate> logicGates;
std::array<bool, constants::MAX_FLAGS> flags;



double tickStart;
void physicsLoop(bool* physicsReady) {
	double maxTickTime = 1.0f/constants::PHYSICS_FREQUENCY;

	tickNumber = 0;
	while (runPhysics) {
		tickStart = glfwGetTime();
		*physicsReady = false;
		player.prevPosition = player.position;

		//1-frame inputs;
		interactKey = keyMap["USE_INTERACT"] && !prevInteract;
		prevInteract = keyMap["USE_INTERACT"];


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
		rollingTPS.push_back(tickrate);
		if constexpr (dev::SHOW_PHYSICS_TICKRATE) {
			std::cout << "Tickrate: " << tickrate << "Hz" << std::endl;
		}
		if constexpr (dev::SHOW_PHYSICS_DT) {
			std::cout << "Tick #" << tickNumber << " took " << std::setprecision(6) << (dt * 1e6f) << "µs / Hypothetical tickrate: " << static_cast<int>(1.0f / dt) << endl;
		}

		*physicsReady = true;
		while (glfwGetTime() - tickStart < maxTickTime) {std::this_thread::yield();}
		tickNumber++;



		if (rollingFPS.size() > constants::MAX_ROLLING_VALUE_QUALITY) {rollingFPS.clear();}
		if (rollingTPS.size() > constants::MAX_ROLLING_VALUE_QUALITY) {rollingTPS.clear();}
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
			&logicGates, &flags,
			&textureNames
		);
	} else {
		utils::Player tmpPlayer;
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &tmpPlayer,
			&(physicsData->visplaneData), &(physicsData->wallData), &(physicsData->displacementData),
			&(physicsData->spriteData), &(physicsData->lightData),
			&(physicsData->textObjectData),
			&logicGates, &flags,
			&textureNames
		);
	}

	graphics::prepareOpenGL(&textureNames, &player);
	{
		std::lock_guard<std::mutex> lock(stateSwapMutex);
		graphicsData = physicsData;
	}
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
			if (functionName == "USE_HEADLAMP" && !keyMap["USE_HEADLAMP"]) {
				headLampEnabled = !headLampEnabled;
			}
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
		&logicGates, &flags,
		&textureNames
	);

	currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;
	currentRenderResolution = glm::ivec2(
		glm::min(display::INITIAL_SCREEN_RESOLUTION.x, desiredRenderResolution.x),
		glm::min(display::INITIAL_SCREEN_RESOLUTION.y, desiredRenderResolution.y)
	);
	currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));


	Window = graphics::initialiseWindow(currentWindowResolution.x, currentWindowResolution.y, "Raycasting-Renderer/GPU");
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

	graphics::prepareOpenGL(&textureNames, &player);
	*graphicsData = *physicsData;
	frame::updateSSBOs(graphicsData, &player);
	double maxFrameTime = 1.0f/utils::configToFloat("VIEW_MAX_FREQ");


	//Threads;
	bool physicsReady;
	physicsThread = std::thread(physicsLoop, &physicsReady);
	tickStart = glfwGetTime();

	frameNumber = 0;
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
		frame::updateSSBOs(localGraphicsData, &player);
		frame::draw(blendingAlpha, localGraphicsData, &player);
		glfwSwapBuffers(Window);


		float dt = glfwGetTime() - frameStart;
		if (utils::configToBool("META_SHOW_DT_CONSOLE")) {
			std::cout << "Frame #" << frameNumber << " took " << std::setprecision(2) << (dt * 1e3f) << "ms / Hypothetical framerate: " << static_cast<int>(1.0f / dt) << endl;
		}
		if (!vsync) {
			while (glfwGetTime() - frameStart < maxFrameTime) {std::this_thread::yield();}
		}
		framerate = floor(1.0f / (glfwGetTime() - frameStart));
		rollingFPS.push_back(framerate);
		if (utils::configToBool("META_SHOW_FRAMERATE_CONSOLE")) {
			std::cout << "Framerate: " << framerate << "Hz" << std::endl;
		}

		cursorXPosPrev = cursorXPos;
		cursorYPosPrev = cursorYPos;
		frameNumber++;
	}

	//Cleanup OpenGL.
	glDeleteTextures(1, &GLIndex::textureArrayEnvironment);

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
