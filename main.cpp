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
bool interactKey = false, prevInteract = false, shouldTakeScreenshot = false;
int lightFlickerRNG;
GLuint renderedFrameID, interfaceID, positionMapID, normalMapID, shadowMapID;
GLuint raycastShader, envShader, spriteShader, lightingShader, uiShader, displayShader; //Shaders
GLuint textureArrayEnvironment, skyboxTextureID, textureArrayUI, textureArrayNumeric; //Textures
GLuint wallIntersectSSBO, allVisplanesSSBO, allWallsSSBO, spriteSSBO, lightSSBO, displacementSSBO, visibleVisplaneIndicesSSBO, visibleWallIndicesSSBO; //Storage Buffers
GLuint VAO, uiVAO, uiVBO, uiEBO;
glm::mat4 pvmMatrix;
//Data must be synced between the graphics and physics threads.
utils::DataSet stateA, stateB;
utils::DataSet* physicsData = &stateA;
utils::DataSet* graphicsData = &stateB;
std::mutex stateSwapMutex;
std::vector<float> rollingFPS;
std::vector<float> rollingTPS;



float getAverage(std::vector<float>& q) {
	float n = 0.0f;
	float sum = 0.0f;
	for (float v : q) {
		sum += v;
		n++;
	}
	return sum / n;
}


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


	wallIntersectSSBO = render::createShaderStorageBufferObject(
		7, sizeof(utils::WallIntersect) * currentRenderResolution.x * validWalls
	);

	renderedFrameID = render::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	positionMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	normalMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	shadowMapID = render::createGLImage2D(currentShadowResolution.x, currentShadowResolution.y, GL_RGBA32F, GL_LINEAR);
	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
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

	if (dev::PAUSE_ON_OPENGL_ERROR) {
		utils::pause();
	}
}



std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;
size_t currentVertexSize, currentIndexSize;
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


	allVisplanesSSBO = render::createShaderStorageBufferObject(
		0, sizeof(utils::VisplaneGPU) * validVisplanes
	);
	allWallsSSBO = render::createShaderStorageBufferObject(
		1, sizeof(utils::WallGPU) * validWalls
	);
	spriteSSBO = render::createShaderStorageBufferObject(
		2, sizeof(utils::SpriteGPU) * validSprites
	);
	lightSSBO = render::createShaderStorageBufferObject(
		3, sizeof(utils::LightGPU) * validLights
	);
	displacementSSBO = render::createShaderStorageBufferObject(
		4, sizeof(utils::DisplacementGPU) * validDisplacements
	);
	visibleVisplaneIndicesSSBO = render::createShaderStorageBufferObject(
		5, sizeof(uint) * validVisplanes
	);
	visibleWallIndicesSSBO = render::createShaderStorageBufferObject(
		6, sizeof(uint) * validWalls
	);
	wallIntersectSSBO = render::createShaderStorageBufferObject(
		7, sizeof(utils::WallIntersect) * currentRenderResolution.x * validWalls
	);


	//Raycast compute shader
	raycastShader = render::createComputeShader("raycast");

	//Environment shader
	envShader = render::createShaderProgram("environment", false);

	//Sprite Shader
	spriteShader = render::createShaderProgram("sprites", false);

	//Shadow Shader
	lightingShader = render::createShaderProgram("lighting", false);

	//uiShader
	uiShader = render::createShaderProgram("interface");

	//Display Shader
	displayShader = render::createShaderProgram("display");



	glGenVertexArrays(1, &uiVAO);
	glBindVertexArray(uiVAO);

	glGenBuffers(1, &uiVBO);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	glBufferData(GL_ARRAY_BUFFER, constants::MAX_VERTEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentVertexSize = constants::MAX_VERTEX_BYTES;

	glGenBuffers(1, &uiEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, constants::MAX_INDEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentIndexSize = constants::MAX_INDEX_BYTES;

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);



	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	VAO = render::getVAO();

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
	pvmMatrix = glm::ortho(0.0f, float(display::UI_RESOLUTION.x), 0.0f, float(display::UI_RESOLUTION.y), -1.0f, 1.0f);
	

	//Debug settings
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openGLErrorCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	utils::GLErrorcheck("Initialisation", true); //Old basic debugging
}


float avgframerate, avgtickrate;
bool shouldShowFPS, shouldShowTPS;
//UI
const std::vector<utils::UIElement> UIElements = {
	UIElement(glm::vec2(-16, -72), glm::vec2(192, 192), static_cast<GLuint>(0)),	//Health image
	UIElement(glm::vec2(32, 32), glm::vec2(40, 40), &(player.health)),				//Health number
	UIElement(glm::vec2(780, -72), glm::vec2(192, 192), static_cast<GLuint>(1)), 	//Energy image
	UIElement(glm::vec2(840, 32), glm::vec2(40, 40), &(player.energy)),				//Energy number
	UIElement(glm::vec2(0, 508), glm::vec2(32, 32), &avgframerate, &shouldShowFPS),	//FPS number
	UIElement(glm::vec2(0, 476), glm::vec2(32, 32), &avgtickrate, &shouldShowTPS)	//TPS number
};
GLuint currentIdx;

inline void addImage(glm::vec2 position, glm::vec2 scale, GLuint textureID, bool isAlphaNumeric, bool hasBackground, std::vector<float>* verticesData, std::vector<GLuint>* indicesData, float distance=0.0f) {
	float texID = (textureID << 2) | int(isAlphaNumeric) | int(hasBackground);
	verticesData->insert(verticesData->end(), {
		position.x, 			position.y, 			0.0f, 1.0f, texID, distance,
		position.x, 			position.y + scale.y, 	0.0f, 0.0f, texID, distance,
		position.x + scale.x, 	position.y, 			1.0f, 1.0f, texID, distance,
		position.x + scale.x, 	position.y + scale.y, 	1.0f, 0.0f, texID, distance,
	});

	indicesData->insert(indicesData->end(), {
		currentIdx + 0, currentIdx + 1, currentIdx + 2,
		currentIdx + 1, currentIdx + 2, currentIdx + 3
	});
	currentIdx += 4;
}

void drawInt(glm::vec2 position, glm::vec2 scale, int value, std::vector<float>* verticesData, std::vector<GLuint>* indicesData) { //Values [-99999 <-> 99999] inclusive.
	int absVal = abs(value);
	int maxDigits = 5;
	bool started = false;
	int divisor = 10000;

	glm::vec2 digitOffset = glm::vec2(scale.x * 0.65f, 0.0);


	if (value < 0) {
		addImage(position, scale, 10, true, false, verticesData, indicesData); //"-"
		position += digitOffset;
	}

	for (int i = 0; i < maxDigits; i++) {
		int digit = (absVal / divisor) % 10;

		if (digit > 0 || started || (i == maxDigits - 1)) {
			started = true;
			addImage(position, scale, digit, true, false, verticesData, indicesData); //"[0-9]"
			position += digitOffset;
		}

		divisor /= 10;
	}
}

void drawHUD() {
	currentIdx = 0;
	glClearTexImage(interfaceID, 0, GL_RGBA, GL_FLOAT, nullptr);
	std::vector<float> vertices;
	std::vector<GLuint> indices;
	std::vector<float> verticesData;
	std::vector<GLuint> indicesData;

	shouldShowFPS = utils::configToBool("META_SHOW_FRAMERATE_UI");
	shouldShowTPS = utils::configToBool("META_SHOW_TICKRATE_UI");


	for (const utils::UIElement element : UIElements) {
		if ((element.showPtr) && !*(element.showPtr)) {continue;}

		verticesData.clear();
		indicesData.clear();

		if (element.iPtr) { //Shows an integer value
			drawInt(element.position, element.scale, *(element.iPtr), &verticesData, &indicesData);
		} else if (element.fPtr) { //Shows an integer value
			drawInt(element.position, element.scale, int(*(element.fPtr)), &verticesData, &indicesData);
		} else { //Shows some UI image element
			addImage(element.position, element.scale, element.textureID, false, false, &verticesData, &indicesData);
		}

		utils::combineVectors(&vertices, verticesData);
		utils::combineVectors(&indices, indicesData);
	}

	for (utils::TextObject thisTO : graphicsData->textObjectData) {
		verticesData.clear();
		indicesData.clear();


		float distance = glm::length(glm::vec2(thisTO.position - player.position));
		float scale = thisTO.scale * zoomEffect / distance;

		float centreX = utils::getCentreX(thisTO.position, &player, display::UI_RESOLUTION);
		float projCentreY = (player.cameraPosition.z - thisTO.position.z) * zoomEffect / distance;
		float centreY = display::UI_RESOLUTION.y * (0.5f - projCentreY);
		float charY = centreY - (scale / 2.0f);
		float pitchDecimal = glm::clamp(player.viewPitch, -22.5f, 22.5f) * zoomEffect;
		charY += (pitchDecimal * display::UI_RESOLUTION.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.

		int letterIdx = 0;
		int textLength = thisTO.text.size();
		for (char ch : thisTO.text) {
			//Iterate through letters.
			int charIdx = utils::convertTextToIdx(ch, &symbolNames);
			letterIdx++;

			if (charIdx < 0) {continue; /* Blank Character */}
			float charX = centreX + scale*0.65f*(letterIdx - (textLength/2.0f));
			glm::vec2 charPos = glm::vec2(charX, charY);


			addImage(charPos, glm::vec2(scale, scale), charIdx, true, true, &verticesData, &indicesData, distance);
		}

		utils::combineVectors(&vertices, verticesData);
		utils::combineVectors(&indices, indicesData);
	}


	size_t newVertexSize = vertices.size() * sizeof(float);
	size_t newIndexSize = indices.size() * sizeof(GLuint);
	//Draw using 2D projection (pvmMatrix is just ortho projection.)
	glUseProgram(uiShader);
	glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
	if (newVertexSize > currentVertexSize) {
		glBufferData(GL_ARRAY_BUFFER, newVertexSize, vertices.data(), GL_DYNAMIC_DRAW);
		currentVertexSize = newVertexSize;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, newVertexSize, vertices.data());
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiEBO);
	if (newIndexSize > currentIndexSize) {
		glBufferData(GL_ARRAY_BUFFER, newIndexSize, vertices.data(), GL_DYNAMIC_DRAW);
		currentIndexSize = newIndexSize;
	} else {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, newIndexSize, indices.data());
	}

	glBindImageTexture(0, interfaceID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindTextureUnit(0, textureArrayUI);
	glBindTextureUnit(1, textureArrayNumeric);
	glBindTextureUnit(2, renderedFrameID);

	render::bindCommonUniforms(uiShader, &player);
	GLint pvmMatrixLocation = glGetUniformLocation(uiShader, "pvmMatrix");
	glUniformMatrix4fv(pvmMatrixLocation, 1, GL_FALSE, glm::value_ptr(pvmMatrix));

	glBindVertexArray(uiVAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	utils::GLErrorcheck("Interface", true);

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



	//Raycasting compute shader.
	const glm::uvec3 LOCAL_SIZE = glm::uvec3(32, 1, 1);
	glUseProgram(raycastShader);
	render::bindCommonUniforms(raycastShader, &player);
	glDispatchCompute((currentRenderResolution.x + LOCAL_SIZE.x - 1) / LOCAL_SIZE.x, (numVisibleWalls + LOCAL_SIZE.y - 1) / LOCAL_SIZE.y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


	//Environment Shader.
	glUseProgram(envShader);
	glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, textureArrayEnvironment);
	glBindTextureUnit(1, skyboxTextureID);

	//Uniforms
	render::bindCommonUniforms(envShader, &player);
	render::bindUniformValue(envShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));
	render::bindUniformValue(envShader, "allowTransparency", utils::configToBool("VIEW_ALLOW_TRANSPARENCY"));

	renderingGeneric("Environment Shader");



	//Sprite Shader.
	glUseProgram(spriteShader);
	glBindImageTexture(0, renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, textureArrayEnvironment);

	//Uniforms
	render::bindCommonUniforms(spriteShader, &player);
	render::bindUniformValue(spriteShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));

	renderingGeneric("Sprite Shader");



	//Lighting Shader
	glViewport(0, 0, currentShadowResolution.x, currentShadowResolution.y);
	glUseProgram(lightingShader);

	glBindTextureUnit(0, positionMapID);
	glBindTextureUnit(1, normalMapID);
	glBindTextureUnit(2, textureArrayEnvironment);
	glBindImageTexture(0, shadowMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//Uniforms;
	render::bindCommonUniforms(lightingShader, &player);
	render::bindUniformValue(lightingShader, "allowTransparency", utils::configToBool("VIEW_ALLOW_TRANSPARENCY") && utils::configToBool("VIEW_ALLOW_TRANSPARENT_SHADOWS"));
	render::bindUniformValue(lightingShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));
	render::bindUniformValue(lightingShader, "headLampEnabled", headLampEnabled);
	render::bindUniformValue(lightingShader, "headLampFlicker", lightFlickerRNG);

	renderingGeneric("Lighting Shader");



	//UI Shader.
	if (utils::configToBool("VIEW_SHOW_HUD")) {
		glViewport(0, 0, display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
		avgframerate = getAverage(rollingFPS);
		avgtickrate = getAverage(rollingTPS);
		drawHUD();
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
	//Display-Specific
	render::bindUniformValue(displayShader, "antiAliasingLevel", utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
	render::bindUniformValue(displayShader, "smoothingEnabled", utils::configToBool("VIEW_SMOOTHING"));
	render::bindUniformValue(displayShader, "quantisingLevel", utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));
	render::bindUniformValue(displayShader, "screenshotHasHUD", utils::configToBool("VIEW_INTERFACE_IN_SCREENSHOT"));

	renderingGeneric("Display Shader");


	if (shouldTakeScreenshot) {
		render::saveScreenshot(renderedFrameID);
	}
}



//Non-synced data.
std::vector<utils::LogicGate> logicGates;
std::array<bool, constants::MAX_FLAGS> flags;


std::vector<uint> visibleVisplaneIndices;
std::vector<uint> visibleWallIndices;
void updateSSBOs(utils::DataSet* localGraphicsData) {
	visibleVisplaneIndices.clear();
	visibleWallIndices.clear();
	render::findVisibleObjects(&player, &(localGraphicsData->visplaneData), &visibleVisplaneIndices, &(localGraphicsData->wallData), &visibleWallIndices);


	//Update SSBOs.
	render::updateShaderStorageBufferObject<utils::VisplaneGPU>(
		allVisplanesSSBO, &player, &(localGraphicsData->visplaneData)
	);
	render::updateShaderStorageBufferObject<utils::WallGPU>(
		allWallsSSBO, &player, &(localGraphicsData->wallData)
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
	render::updateShaderStorageBufferObject<uint>(
		visibleVisplaneIndicesSSBO, &visibleVisplaneIndices
	);
	render::updateShaderStorageBufferObject<uint>(
		visibleWallIndicesSSBO, &visibleWallIndices
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
			std::cout << "Tick #" << tick << " took " << std::setprecision(6) << (dt * 1e6f) << "µs / Hypothetical tickrate: " << static_cast<int>(1.0f / dt) << endl;
		}

		*physicsReady = true;
		while (glfwGetTime() - tickStart < maxTickTime) {std::this_thread::yield();}
		tick++;



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
		rollingFPS.push_back(framerate);
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
