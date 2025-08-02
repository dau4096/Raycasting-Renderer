#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define GL_DEBUG_OUTPUT

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
bool headLampEnabled = false;
bool interactKey = false, shouldTakeScreenshot = false;
int lightFlickerRNG;
GLuint skyShader, triShader; //Shaders
GLuint textureArrayEnvironment, skyboxTextureID, textureArrayUI, textureArrayNumeric; //Textures


void framebufferSizeCallback(GLFWwindow* Window, int width, int height) {
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	currentWindowResolution = glm::ivec2(width, height);

	verticalFOV = 2 * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentWindowResolution.y) / float(currentWindowResolution.x)));
	aspectRatio = float(width) / float(height);
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



std::vector<utils::Visplane> visplaneData;
std::vector<utils::Wall> wallData;
std::vector<utils::Displacement> displacementData;
std::vector<utils::Sprite> spriteData;
std::vector<utils::Light> lightData;
std::vector<utils::TextObject> textObjectData;
size_t numTris;


std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;
void prepareOpenGL() {
	//OpenGL setup;
	textureArrayEnvironment = render::createTexture2DArray(textureNames, "textures-env", true);
	textureArrayUI = render::createTexture2DArray(UIImageNames, "textures-sym");
	textureArrayNumeric = render::createTexture2DArray(symbolNames, "textures-sym");
	skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);

	skyShader = render::createShaderProgram("sky", false);
	triShader = render::createShaderProgram("triangle", true);

	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_BLEND);

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentWindowResolution.y) / float(currentWindowResolution.x)));
	aspectRatio = float(currentWindowResolution.x) / float(currentWindowResolution.y);

	//Debug settings
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openGLErrorCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	utils::GLErrorcheck("Initialisation", true); //Old basic debugging
}


void renderFrame() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Create VAOs;
	GLuint screenspaceVAO = render::getVAO();
	GLuint envVAO = render::createVAO(&visplaneData, &wallData, &displacementData, &spriteData, &textObjectData, &numTris, &player);

	//Matrices
	float viewBob = (utils::configToBool("VIEW_BOB")) ? render::viewBob(tick, player) : 0.0f;
	player.cameraPosition = player.position + glm::vec3(0.0f, 0.0f, (player.height/3.0f) + viewBob);
	float yawRadians = player.viewAngle * constants::TO_RAD;
	float pitchRadians = player.viewPitch * constants::TO_RAD;
	glm::vec3 viewDirection = glm::vec3(
		sin(yawRadians) * cos(pitchRadians),
		cos(yawRadians) * cos(pitchRadians),
		sin(pitchRadians)
	);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::lookAt(player.cameraPosition, player.cameraPosition + viewDirection, glm::vec3(0.0f, 0.0f, 1.0f)); //+Z is up.

	//Perspective matrix for XY.
	glm::mat4 projMatrix = glm::perspective(utils::configToFloat("VIEW_FOV") / zoomEffect, aspectRatio, display::Z_NEAR, utils::configToFloat("VIEW_MAX_DIST"));
	glm::mat4 perspectiveMatrix = projMatrix * viewMatrix * modelMatrix;

	//Orthographic matrix for Z.
	float orthoTopZ = player.cameraPosition.z + utils::configToFloat("VIEW_MAX_DIST");
	float orthoBottomZ = player.cameraPosition.z - utils::configToFloat("VIEW_MAX_DIST");

	glm::mat4 orthoZ = glm::ortho(
	    -1.0f, 1.0f, -1.0f, 1.0f, //XY not used in shader, only Z.
	    orthoBottomZ, orthoTopZ
	);

	glm::mat4 orthoMatrix = orthoZ * viewMatrix * modelMatrix;


	//Sky shader
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(skyShader);
	glBindTextureUnit(0, skyboxTextureID);

	//Uniforms
	render::bindCommonUniforms(skyShader, &player);

	glBindVertexArray(screenspaceVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	utils::GLErrorcheck("Sky Shader", true);


	//Environment triangle Shader.
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(triShader);

	glBindTextureUnit(0, textureArrayEnvironment);

	//Uniforms
	render::bindCommonUniforms(triShader, &player);

	//MVPMatrix;
	GLint perspectiveMatrixLocation = glGetUniformLocation(triShader, "perspectiveMatrix");
	glUniformMatrix4fv(perspectiveMatrixLocation, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
	GLint orthoMatrixLocation = glGetUniformLocation(triShader, "orthoMatrix");
	glUniformMatrix4fv(orthoMatrixLocation, 1, GL_FALSE, glm::value_ptr(orthoMatrix));

	//Other
	render::bindUniformValue(triShader, "headLampEnabled", headLampEnabled);
	render::bindUniformValue(triShader, "headLampFlicker", lightFlickerRNG);
	render::bindUniformValue(triShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));

	glBindVertexArray(envVAO);
	glDrawElements(GL_TRIANGLES, (GLsizei)numTris, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	utils::GLErrorcheck("Triangle Shader", true);

	glfwSwapBuffers(Window);
}





std::vector<utils::LogicGate> logicGates;
std::array<int, constants::MAX_FLAGS> flags;

void computeFrame(bool* CPUDone) {
	//Update logic states.
	for (int index=0; index<validGates; index++) {
		LogicGate gate = logicGates[index];
		if (gate.gateType == G_INVALID) {continue;}
		gate.evaluateState();
		logicGates[index] = gate;
	}
	physics::updateSpecials(&wallData, &visplaneData, &player, interactKey);

	physics::playerMove(&player, &wallData, &spriteData, &visplaneData);
	*CPUDone = true;
}



double cursorXPos, cursorYPos, cursorXPosPrev, cursorYPosPrev;
static inline void reloadLevel(const bool resetPlayer=false) {
	if (resetPlayer) {
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &player,
			&visplaneData, &wallData, &displacementData,
			&spriteData, &lightData,
			&textObjectData,
			&logicGates, &flags,
			&textureNames
		);
	} else {
		utils::Player tmpPlayer;
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &tmpPlayer,
			&visplaneData, &wallData, &displacementData,
			&spriteData, &lightData,
			&textObjectData,
			&logicGates, &flags,
			&textureNames
		);
	}
	if (utils::configToBool("META_DYNAMIC_UPD_ALLOW_NEW_TEXTURES")) {
		glDeleteTextures(1, &textureArrayEnvironment);
		textureArrayEnvironment = render::createTexture2DArray(textureNames);
		skyboxTextureID = render::loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);
	}
}

void handleInputs() {
	glfwPollEvents();

	//Get inputs for this frame
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
		player.vLook = glm::clamp(float(player.vLook-dY), -89.00f, 89.00f);
	}
}




int main() {
	try { //Catch exceptions
	loader::loadBindings();
	loader::loadStage(
		userConfig["META_STAGE_NAME"], &player,
		&visplaneData, &wallData, &displacementData,
		&spriteData, &lightData,
		&textObjectData,
		&logicGates, &flags,
		&textureNames
	);


	currentWindowResolution = display::INITIAL_SCREEN_RESOLUTION;


	Window = render::initializeWindow(currentWindowResolution.x, currentWindowResolution.y, "Raycasting-Renderer/GPU");
	glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
	glfwGetCursorPos(Window, &cursorXPos, &cursorYPos);
	bool vsync = utils::configToBool("VIEW_VSYNC");
	if (vsync) {
		glfwSwapInterval(1);
	}

	cursorXPosPrev = cursorXPos;
	cursorYPosPrev = cursorYPos;
	utils::GLErrorcheck("Window Creation", true);

	prepareOpenGL();
	double maxFrameTime = 1.0f/utils::configToFloat("VIEW_MAX_FREQ");


	//Threads;
	bool CPUDone;


	while (!glfwWindowShouldClose(Window)) {
		tick++;
		CPUDone = false;

		double frameStart = glfwGetTime();
		handleInputs();
		if (keyMap["META_EXIT"]) {break; /* Quit Immediately */}

		std::thread threadCPU(computeFrame, &CPUDone);
		threadCPU.join();
		renderFrame();
		while (!(CPUDone)) {std::this_thread::yield();}


		if (!vsync) {
			while (glfwGetTime() - frameStart < maxFrameTime) {std::this_thread::yield();}
		}
		freq = floor(1.0f / (glfwGetTime() - frameStart));
		if (utils::configToBool("META_SHOW_FREQ_CONSOLE")) {
			std::cout << freq << std::endl;
		}

		cursorXPosPrev = cursorXPos;
		cursorYPosPrev = cursorYPos;
	}

	//Cleanup OpenGL.
	glDeleteTextures(1, &textureArrayEnvironment);


	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;


	//Catch exceptions.
	} catch (const std::exception& e) {
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An exception was thrown: " << e.what() << std::endl;
		pause();
		return -1;
	} catch (...) {
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		std::cerr << "An unspecified exception was thrown." << std::endl;
		pause();
		return -1;
	}
}
