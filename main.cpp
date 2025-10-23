#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma execution_character_set("utf-8")


#include <stb_image.h>
#include <stb_image_write.h>

#include "src/includes.h"

#include "src/constants.h"
#include "src/global.h"

#include "src/utils.h"
#include "src/console.h"

#include "src/loader.h"
#include "src/physics.h"
#include "src/graphics.h"


using namespace std;
using namespace utils;
using namespace glm;


void framebufferSizeCallback(GLFWwindow* Window, int width, int height) {
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentWindowResolution = glm::ivec2(width, height);
	currentRenderResolution = glm::min(currentWindowResolution, desiredRenderResolution);
	currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));

	//SSBOs
	GLIndex::wallIntersectSSBO = graphics::createShaderStorageBufferObject(
		7, sizeof(structs::WallIntersect) * currentRenderResolution.x * validWalls
	);

	//Image2Ds
	GLIndex::lightingMapsArrayID = graphics::createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);
	GLIndex::screenshotImage2D = graphics::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);

	//Framebuffers
	GLIndex::frameFBO = graphics::createEnvironmentFBO(currentRenderResolution);
	GLIndex::displacementFBO = graphics::createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
}






double tickStart;
void physicsLoop() {
	double maxTickTime = 1.0f/constants::PHYSICS_FREQUENCY;

	tickNumber = 0;
	while (runPhysics) {
		tickStart = glfwGetTime();
		player.prevPosition = player.position;

		//1-frame inputs;
		interactKey = utils::isPressed("USE_INTERACT") && !prevInteract;
		prevInteract = utils::isPressed("USE_INTERACT");


		//Update logic states.
		for (int index=0; index<validGates; index++) {
			structs::LogicGate* gate = &(logicGates[index]);
			if (gate->gateType == G_INVALID) {continue;}
			gate->evaluateState();
		}
		physics::updateSpecials(interactKey);
		physics::updatePhysicsObjects();
		physics::playerMovement();


		//Update states;
		screenTint = graphics::manageScreenTint();
		player.previousState = player.state;
		player.state = E_NONE;
		{
			std::lock_guard<std::mutex> lock(stateSwapMutex);
			std::swap(physicsData, graphicsData);
		}

		while (glfwGetTime() - tickStart < maxTickTime) {std::this_thread::yield();}
		
		float dt = glfwGetTime() - tickStart;
		tickrate = floor(1.0f / dt);
		if (!shouldTakeScreenshot) {rollingTPS.push_back(tickrate);}

		if constexpr (dev::SHOW_PHYSICS_TICKRATE) {
			std::cout << "Tickrate: " << tickrate << "Hz" << std::endl;
		}
		if constexpr (dev::SHOW_PHYSICS_DT) {
			std::cout << "Tick #" << tickNumber << " took " << std::setprecision(6) << (dt * 1e6f) << "µs / Hypothetical tickrate: " << static_cast<int>(1.0f / dt) << endl;
		}


		tickNumber++;



		if (rollingFPS.size() > constants::MAX_ROLLING_VALUE_QUALITY) {rollingFPS.clear();}
		if (rollingTPS.size() > constants::MAX_ROLLING_VALUE_QUALITY) {rollingTPS.clear();}
	}
}


void consoleLoop() {
	//Simple loop for console input.
	while (runConsole) {
		std::string line;
		std::cout << "> ";
		std::getline(std::cin, line);
		console::exec(line);
	}
}




double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;

void handleInputs() {
	glfwPollEvents();

	bool lastFrameScreenshot = utils::isPressed("META_SCREENSHOT");

	//Get inputs for this frame
	for (auto &pair : userBindings) {
		std::string functionName = pair.first;
		int keyEnum = pair.second;
		if (keyEnum == -1) {
			std::cout << functionName << " was not bound to a key!" << std::endl;
			userBindings[functionName] = -2; //Do not warn user multiple times.
		}
		if (keyEnum < 0) {continue;}

		int keyState = glfwGetKey(Window, keyEnum);
		if (keyState == GLFW_PRESS) {
			if (functionName == "USE_HEADLAMP" && !utils::isPressed("USE_HEADLAMP")) {
				headLampEnabled = !headLampEnabled;
			}
			utils::setPressed(functionName, true);

		} else if (keyState == GLFW_RELEASE) {
			utils::setPressed(functionName, false);
		}
	}


	//Meta controls
	if (utils::isPressed("META_FREECURSOR")) {
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	} else {
		glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	}

	shouldTakeScreenshot = utils::isPressed("META_SCREENSHOT") && !shouldTakeScreenshot && !lastFrameScreenshot;
	zoomEffect = (utils::isPressed("USE_VIEWZOOM")) ? display::ZOOM_MULT : 1.0f;

	if (utils::isPressed("META_RELOAD_STAGE")) {
		stage::reload(true);
	} else if (utils::isPressed("META_RELOAD_ENV") || utils::configToBool("META_DYNAMIC_UPD")) {
		stage::reload(false);
	}


	//Crouch changes physical height
	if (utils::isPressed("MOVE_CROUCH")) {
		player.height = playerConfig::PLAYER_COLLISION_HEIGHT_CROUCH;
		player.touchingFloor = false;
	} else {
		player.height = playerConfig::PLAYER_COLLISION_HEIGHT_STAND;
	}







	rayAngle = (utils::isPressed("USE_VIEWZOOM")) ? utils::configToFloat("VIEW_FOV")/(display::ZOOM_MULT * 2.0f) : utils::configToFloat("VIEW_FOV")/2.0f;
	rayAngle *= constants::TO_RAD;
	zoomEffect = ((utils::isPressed("USE_VIEWZOOM")) ? display::ZOOM_MULT : 1.0f);
	bool useVLOOK = utils::configToBool("VIEW_VLOOK");

	//Mouse camera controls;
	double cursorXDelta = cursorXPos - cursorXPosPrev;
	double cursorYDelta = cursorYPos - cursorYPosPrev;
	player.viewAngle += cursorXDelta * constants::TO_RAD * (utils::configToFloat("TURN_SPEED_MOUSE") / zoomEffect);
	if (useVLOOK) {
		player.vLook += cursorYDelta * constants::TO_RAD * (utils::configToFloat("TURN_SPEED_MOUSE") / zoomEffect);
	}



	//Keyboard camera controls;
	float keyboardTurnSpeed = constants::TO_RAD * utils::configToFloat("TURN_SPEED_KEYBOARD") / zoomEffect;
	if (utils::isPressed("CAMERA_YAW_LEFT")) {
		player.viewAngle -= keyboardTurnSpeed;
	}
	if (utils::isPressed("CAMERA_YAW_RIGHT")) {
		player.viewAngle += keyboardTurnSpeed;
	}
	if (useVLOOK && utils::isPressed("CAMERA_PITCH_UP")) {
		player.vLook -= keyboardTurnSpeed;
	}
	if (useVLOOK && utils::isPressed("CAMERA_PITCH_DOWN")) {
		player.vLook += keyboardTurnSpeed;
	}


	player.viewAngle = fmodf(player.viewAngle + constants::PI*3.0f, constants::PI2) - constants::PI;
	player.vLook = glm::clamp(player.vLook, -0.125f*constants::PI, 0.125f*constants::PI);
}


std::thread physicsThread;
inline void stopPhysics() {
	runPhysics = false;
	if (physicsThread.joinable()) {
		physicsThread.join();
	}
}
std::thread consoleThread;
inline void stopConsole() {
	runConsole = false;
	if (consoleThread.joinable()) {
		consoleThread.join();
	}
}

int main() {
	try { //Catch exceptions

#ifdef __WIN32
	SetConsoleOutputCP(65001); //CP_UTF8
#endif

	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		physicsData, &logicGates
	);
	player.state = E_RESPAWN;

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

	graphics::prepareOpenGL();
	*graphicsData = *physicsData;
	frame::updateSSBOs(true);
	double maxFrameTime = 1.0f/utils::configToFloat("VIEW_MAX_FREQ");


	//Threads;
	physicsThread = std::thread(physicsLoop);
	consoleThread = std::thread(consoleLoop);


	tickStart = glfwGetTime();
	frameNumber = 0;
	while (!glfwWindowShouldClose(Window)) {
		double frameStart = glfwGetTime();
		double blendingAlpha = (frameStart - tickStart) * constants::PHYSICS_FREQUENCY;

		handleInputs();
		if (utils::isPressed("META_EXIT")) {break; /* Quit Immediately */}


		graphics::handleTextureLoadQueue();
		frame::draw(blendingAlpha, frameStart);
		glfwSwapBuffers(Window);
		glFinish();

		float dt = glfwGetTime() - frameStart;
		if (utils::configToBool("META_SHOW_DT_CONSOLE")) {
			std::cout << "Frame #" << frameNumber << " took " << std::setprecision(2) << (dt * 1e3f) << "ms / Hypothetical framerate: " << static_cast<int>(1.0f / dt) << endl;
		}
		if (!vsync) {
			while (glfwGetTime() - frameStart < maxFrameTime) {std::this_thread::yield();}
		}
		framerate = floor(1.0f / (glfwGetTime() - frameStart));
		if (!shouldTakeScreenshot) {rollingFPS.push_back(framerate);}
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
	stopConsole();
	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;


	//Catch exceptions.
	} catch (const std::exception& e) {
		stopPhysics();
		stopConsole();
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An exception was thrown: " << e.what() << std::endl;
		utils::pause();
		return -1;
	} catch (...) {
		stopPhysics();
		stopConsole();
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An unspecified exception was thrown." << std::endl;
		utils::pause();
		return -1;
	}
}
