#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma execution_character_set("utf-8")

#include <stb_image.h>
#include <stb_image_write.h>
#include "src/includes.h"
#include "src/global.h"
#include "src/loader.h"
#include "src/physics.h"
#include "src/graphics.h"
#include "src/utils.h"
using namespace std;
using namespace utils;
using namespace glm;


//// CALLBACKS ////
//framebufferSizeCallback but for the terminal render mode instead.
void handleWinChange(int sig) {
	//The console may have changed size.
	currentConsoleResolution = utils::getConsoleResolution();
	if (utils::configToBool("SCREEN_CONSOLE_RENDER")) {
		currentRenderResolution = currentConsoleResolution;
		currentWindowResolution = currentConsoleResolution;
		desiredRenderResolution = currentConsoleResolution;
		if (lightingType == LIGHT_DYNAMIC) {currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));}

		//SSBOs
		GLIndex::wallIntersectSSBO = graphics::createShaderStorageBufferObject(
			7, sizeof(structs::WallIntersect) * currentRenderResolution.x * validWalls
		);

		//Image2Ds
		GLIndex::lightingMapsArrayID = graphics::createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);
		GLIndex::finishedFrame = graphics::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);

		//Framebuffers
		GLIndex::frameFBO = graphics::createEnvironmentFBO(currentRenderResolution);
		GLIndex::displacementFBO = graphics::createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);

		verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
	}
}

GLFWwindow* Window;

void framebufferSizeCallback(GLFWwindow* Window, int width, int height) {
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentWindowResolution = glm::ivec2(width, height);
	currentRenderResolution = glm::ivec2(
		glm::min(width, desiredRenderResolution.x),
		glm::min(height, desiredRenderResolution.y)
	);
	if (lightingType == LIGHT_DYNAMIC) {
		currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));
	}

	//SSBOs
	GLIndex::wallIntersectSSBO = graphics::createShaderStorageBufferObject(
		7, sizeof(structs::WallIntersect) * currentRenderResolution.x * validWalls
	);

	//Image2Ds
	GLIndex::finishedFrame = graphics::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLIndex::lightingMapsArrayID = graphics::createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);

	//Framebuffers
	GLIndex::frameFBO = graphics::createEnvironmentFBO(currentRenderResolution);
	GLIndex::displacementFBO = graphics::createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));


	//The console may have changed size.
	currentConsoleResolution = utils::getConsoleResolution();
}
//// CALLBACKS ////



//Non-synced data.
std::vector<utils::LogicGate> logicGates;



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
			LogicGate gate = logicGates[index];
			if (gate.gateType == G_INVALID) {continue;}
			gate.evaluateState();
			logicGates[index] = gate;
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

		if (utils::configToBool("META_SHOW_DATA")) {
			std::cout << "#" << std::to_string(tickNumber) << ":" << std::endl;
			std::cout << "  FPS: " << std::setw(4) << framerate << "Hz" << "    HEALTH: " << std::setw(3) << std::to_string(player.health) << "    ENERGY: " <<std::setw(3) << std::to_string(player.health) << std::endl;
			std::cout << "  POS: (" << std::setw(8) << player.position.x << ", " << std::setw(8) << player.position.y << ", " << std::setw(8) << player.position.z << ")" << std::endl;
			std::cout << "  ANG: ("<< std::setw(8) << player.viewAngle << ", "<< std::setw(8) << player.viewPitch << ", "<< std::setw(8) << player.viewRoll << ")" << std::endl;
			std::cout << "  VEL: (" << std::setw(8) << player.velocity.x << ", " << std::setw(8) << player.velocity.y << ", " << std::setw(8) << player.velocity.z << ")\n" << std::endl;
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



double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
inline void reloadLevel(const bool resetPlayer=false) {
	if (resetPlayer) {
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &player,
			physicsData, &logicGates
		);
	} else {
		structs::Player tmpPlayer;
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &tmpPlayer,
			physicsData, &logicGates
		);
	}

	graphics::prepareOpenGL();
	{
		std::lock_guard<std::mutex> lock(stateSwapMutex);
		graphicsData = physicsData;
	}
}

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
		reloadLevel(true);
	} else if (utils::isPressed("META_RELOAD_ENV") || utils::configToBool("META_DYNAMIC_UPD")) {
		reloadLevel(false);
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

	//// CAMERA INPUT ////
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


	//Gamepad camera controls;
	glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepadState);
	if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
		std::cout << "Gamepad connected: " << glfwGetJoystickName(GLFW_JOYSTICK_1) << std::endl; //Why does it detect an LED controller??
		glm::vec2 gamepadDelta = utils::applyDeadzone(glm::vec2(
			gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_X], gamepadState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]
		)); //Use right axis input.
		player.viewAngle += gamepadDelta.x * constants::TO_RAD * (utils::configToFloat("TURN_SPEED_GAMEPAD") / zoomEffect);
		if (useVLOOK) {
			player.vLook += gamepadDelta.y * constants::TO_RAD * (utils::configToFloat("TURN_SPEED_GAMEPAD") / zoomEffect);
		}
	}
	//// CAMERA INPUT ////


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

int main() {
	try { //Catch exceptions

#ifdef __WIN32
	SetConsoleOutputCP(65001); //CP_UTF8
#else
	setlocale(LC_ALL, "C.UTF-8");
#endif

	currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;
	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		physicsData, &logicGates
	);
	player.state = E_RESPAWN; //Player initial state, uses the screenspace effect associated with E_RESPAWN.


	if (utils::configToBool("SCREEN_CONSOLE_RENDER")) {
		currentConsoleResolution = utils::getConsoleResolution();

		currentRenderResolution = currentConsoleResolution; currentWindowResolution = currentConsoleResolution; desiredRenderResolution = currentConsoleResolution;
		currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));
	} else {
		currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;
		currentConsoleResolution = utils::getConsoleSizeChars();
		currentRenderResolution = glm::ivec2(
			glm::min(display::INITIAL_SCREEN_RESOLUTION.x, desiredRenderResolution.x),
			glm::min(display::INITIAL_SCREEN_RESOLUTION.y, desiredRenderResolution.y)
		);
		currentShadowResolution = glm::ivec2(glm::vec2(currentRenderResolution) * utils::configToFloat("VIEW_SHADOW_QUALITY"));
	}


	Window = graphics::initialiseWindow(currentWindowResolution.x, currentWindowResolution.y, "Raycasting-Renderer/GPU-with-CRT");
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	signal(SIGWINCH, handleWinChange);
	glfwSetJoystickCallback(nullptr); //No callback.
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	glEnable(GL_BLEND);
	bool vsync = utils::configToBool("SCREEN_VSYNC");
	glfwSwapInterval((vsync) ? 1 : 0);




	//Lighting support checks;
	loader::getSupportedExtensions();

	if ((lightingType == LIGHT_STATIC_ARB) && !loader::OpenGLSupportsARB()) {
		utils::print("Attempted to use ARB texturing for shadowmapping. This is unsupported on your hardware. Falling back to lower-fidelity fixed resolution maps.");
		lightingType = LIGHT_STATIC_FIXED; //ARB is not supported; fallback to "old" fixed size method.
	}



	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);

	graphics::prepareOpenGL();
	*graphicsData = *physicsData;
	frame::updateSSBOs(true); //Include W_NODRAW/V_NODRAW objects.
	double maxFrameTime = 1.0d / static_cast<double>(utils::configToFloat("SCREEN_MAX_FREQ"));



	//Create lightmaps if required
	if ((lightingType == LIGHT_STATIC_FIXED) || (lightingType == LIGHT_STATIC_ARB)) {lighting::createLightMaps();}


	//Threads;
	physicsThread = std::thread(physicsLoop);
	tickStart = glfwGetTime();

	//Timer queries;
	GLuint timerQuery;
	GLuint64 GPUnanosecs; //Nanoseconds
	glGenQueries(1, &timerQuery);

	frameNumber = 0u;
	while (!glfwWindowShouldClose(Window)) {
		double frameStart = glfwGetTime();
		double blendingAlpha = (frameStart - tickStart) * constants::PHYSICS_FREQUENCY; //Manages smooth motion when graphics freq > physics tickrate.

		//Handle inputs.
		handleInputs();
		if (utils::isPressed("META_EXIT")) {break; /* Quit Immediately */}


		//Draw this frame
		glBeginQuery(GL_TIME_ELAPSED, timerQuery);
		graphics::handleTextureLoadQueue();
		frame::draw(blendingAlpha, frameStart);
		glEndQuery(GL_TIME_ELAPSED);
		glFinish();


		//Calculate dt and report back if needed.
		float dt = glfwGetTime() - frameStart;
		if (utils::configToBool("META_SHOW_DT_CONSOLE")) {
			glGetQueryObjectui64v(timerQuery, GL_QUERY_RESULT, &GPUnanosecs);
			double ms = GPUnanosecs / 1e6;
			std::cout << "Frame #" << frameNumber << " took " << std::setprecision(2) << ms << "ms / Hypothetical framerate: " << static_cast<int>(1000.0d/ms) << endl;
		}
		//Wait for the correct freq.
		if (!vsync) {while (glfwGetTime() - frameStart < maxFrameTime) {std::this_thread::yield();}}
		glfwSwapBuffers(Window); //Swap to show the new frame, at the correct time.


		framerate = floor(1.0f / (glfwGetTime() - frameStart));
		if (!shouldTakeScreenshot) {rollingFPS.push_back(framerate);} //Screenshots (obviously) cause a lag-spike. Don't "poison" the rolling values with it.
		//Show framerate (One after the mandatory wait-period of the frame.)
		if (utils::configToBool("META_SHOW_FRAMERATE_CONSOLE")) {std::cout << "Framerate: " << framerate << "Hz" << std::endl;}


		//End-of-frame management.
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
		utils::pause();
		return -1;
	} catch (...) {
		stopPhysics();
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An unspecified exception was thrown." << std::endl;
		utils::pause();
		return -1;
	}
}
