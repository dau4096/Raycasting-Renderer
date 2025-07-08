#include "includes.h"
#include "global.h"
#include "utils.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image_write.h"
using namespace std;
using namespace utils;
using namespace glm;




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





GLuint compileShader(GLenum shaderType, string filePath) {
	std::string source = utils::readFile(filePath);
	const char* src = source.c_str();

	GLuint shader = glCreateShader(shaderType);
	if (shader == 0) {
		raise("Error: Failed to create shader.");
		return 0;
	}

	glShaderSource(shader, 1, &src, nullptr);

	glCompileShader(shader);
	

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		char infolog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infolog);
		raise("Error: Shader compilation failed;\n" + string(infolog));
	}

	return shader;
}





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



namespace uniforms {

//Uniforms; [Many overloads]
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, bool value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform1i(location, value);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, size_t value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform1i(location, value);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, int value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform1i(location, value);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, float value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform1f(location, value);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::ivec2 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform2i(location, value.x, value.y);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::vec2 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform2f(location, value.x, value.y);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::ivec3 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform3i(location, value.x, value.y, value.z);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::vec3 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform3f(location, value.x, value.y, value.z);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::ivec4 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform4i(location, value.x, value.y, value.z, value.w);
	}
}
static inline void bindUniformValue(GLuint shaderProgram, const GLchar* uniformName, glm::vec4 value) {
	GLuint location = glGetUniformLocation(shaderProgram, uniformName);
	if (location >= 0) {
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}
}
static void bindCommonUniforms(GLuint shaderProgram, utils::Player* player) {
	//Applies value if shader has uniform of matching name.

	//Camera Data
	bindUniformValue(shaderProgram, "maxRayDistance", utils::configToFloat("VIEW_MAX_RAY_DIST"));
	bindUniformValue(shaderProgram, "maxRayAngle", utils::configToFloat("VIEW_FOV") / 2.0f);
	bindUniformValue(shaderProgram, "verticalFOV", verticalFOV);
	bindUniformValue(shaderProgram, "zoomFactor", display::ZOOM_MULT);
	bindUniformValue(shaderProgram, "zoom", keyMap["USE_VIEWZOOM"]);

	//Player Data
	bindUniformValue(shaderProgram, "playerPosition", player->cameraPosition);
	bindUniformValue(shaderProgram, "playerViewAngle", player->viewAngle);
	bindUniformValue(shaderProgram, "playerViewRoll", player->viewRoll);
	bindUniformValue(shaderProgram, "playerViewPitch", player->viewPitch);

	//Debug
	bindUniformValue(shaderProgram, "debugMode", utils::configToInt("META_DEBUG_MODE"));

	//Sun
	bindUniformValue(shaderProgram, "sunDirection", stageData.sunDirection);
	bindUniformValue(shaderProgram, "sunColour", stageData.sunColour);

	//Other
	bindUniformValue(shaderProgram, "numVisplanes", validVisplanes);
	bindUniformValue(shaderProgram, "numWalls", validWalls);
	bindUniformValue(shaderProgram, "numVisibleVisplanes", numVisibleVisplanes);
	bindUniformValue(shaderProgram, "numVisibleWalls", numVisibleWalls);
	bindUniformValue(shaderProgram, "numDisplacements", validDisplacements);
	bindUniformValue(shaderProgram, "numSprites", validSprites);
	bindUniformValue(shaderProgram, "numLights", validLights);
	bindUniformValue(shaderProgram, "numTextObjects", validTextObjects);
	bindUniformValue(shaderProgram, "shadowMapQuality", 1.0f / utils::configToFloat("VIEW_SHADOW_QUALITY"));

	//Resolutions
	bindUniformValue(shaderProgram, "screenResolution", currentWindowResolution);
	bindUniformValue(shaderProgram, "renderResolution", currentRenderResolution);
	bindUniformValue(shaderProgram, "shadowResolution", currentShadowResolution);
	bindUniformValue(shaderProgram, "interfaceResolution", display::UI_RESOLUTION);
	bindUniformValue(shaderProgram, "skyboxResolution", display::SKYBOX_RESOLUTION);
	bindUniformValue(shaderProgram, "textureResolution", display::TEXTURE_RESOLUTION);
}

}





namespace graphics {

GLFWwindow* initialiseWindow(int width, int height, const char* title) {
	if (!glfwInit()) {
		raise("Failed to initialize GLFW");
		return nullptr;
	}


	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // Set OpenGL major version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);  // Set OpenGL minor version
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // Use Core profile


	GLFWwindow* Window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!Window) {
		glfwTerminate();
		raise("Failed to create GLFW window");
		return nullptr;
	}
	glfwMakeContextCurrent(Window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		raise("Failed to initialize GLEW.");
	}

	glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	return Window;
}



GLuint createShaderProgram(std::string fragShaderName, std::string vertexShaderName="") {
	if (vertexShaderName.empty()) {vertexShaderName = "generic";}
	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, "src\\shaders\\"+ vertexShaderName +".vert");
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "src\\shaders\\"+ fragShaderName +".frag");

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		char infolog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infolog);
		raise("Error: Program linking failed;\n" + string(infolog));
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}




GLuint createComputeShader(std::string compShaderName) {
	GLuint computeShader = compileShader(GL_COMPUTE_SHADER, "src\\shaders\\" + compShaderName + ".comp");

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, computeShader);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		if (!utils::isConsoleVisible()) {
			utils::showConsole();
		}
		char infolog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infolog);
		raise("Error: Compute shader program linking failed:\n" + std::string(infolog));
	}

	glDeleteShader(computeShader);

	return shaderProgram;
}




GLuint createShaderStorageBufferObject(int binding, size_t bufferSize=0, GLuint glType=GL_DYNAMIC_DRAW) {
	GLuint SSBO;
	glGenBuffers(1, &SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, glType);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return SSBO;
}

template<typename TGPU, typename TCPU>
void updateShaderStorageBufferObject(
		GLuint SSBO, utils::Player* player,
		std::vector<TCPU>* dataSetIn
	) {

	size_t singleItemSize = sizeof(TGPU);
	size_t size = dataSetIn->size();
	std::vector<TGPU> dataSet;

	for (size_t index=0; index<size; index++) {
		dataSet.push_back(TGPU(dataSetIn->data() + index, player));
	}

	if (size > 0 && !dataSet.empty()) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, singleItemSize * size, dataSet.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

template<typename TGPU, typename TCPU>
void updateShaderStorageBufferObject(
		GLuint SSBO,
		std::vector<TCPU>* dataSetIn
	) {

	size_t singleItemSize = sizeof(TGPU);
	size_t size = dataSetIn->size();
	std::vector<TGPU> dataSet;

	for (size_t index=0; index<size; index++) {
		dataSet.push_back(TGPU(*(dataSetIn->data() + index)));
	}

	if (size > 0 && !dataSet.empty()) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, singleItemSize * size, dataSet.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

template<typename T>
void updateShaderStorageBufferObject(
	GLuint SSBO,
	T* data,
	size_t count
) {
	size_t size = sizeof(T) * count;
	if (count > 0) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}






void findVisibleObjects(
		utils::Player* player,
		std::vector<utils::Visplane>* visplaneData, std::vector<uint>* visibleVisplaneIndices,
		std::vector<utils::Wall>* wallData, std::vector<uint>* visibleWallIndices
	) {	
	glm::vec2 playerFDirection = glm::vec2(sin(player->viewAngle * constants::TO_RAD), cos(player->viewAngle * constants::TO_RAD));
	glm::vec2 playerPosV2 = glm::vec2(player->position);


	//Visplanes
	for (uint idx=0; idx<visplaneData->size(); idx++) {
		utils::Visplane thisPlane = visplaneData->at(idx);
		bool aProj = glm::dot(thisPlane.start - playerPosV2, playerFDirection) < 0.0f;
		bool bProj = glm::dot(thisPlane.end - playerPosV2, playerFDirection) < 0.0f;
		bool cProj = glm::dot(glm::vec2(thisPlane.start.x, thisPlane.end.y) - playerPosV2, playerFDirection) < 0.0f;
		bool dProj = glm::dot(glm::vec2(thisPlane.end.x, thisPlane.start.y) - playerPosV2, playerFDirection) < 0.0f;
		if (aProj && bProj && cProj && dProj) {continue; /* Completely behind player view */}

		visibleVisplaneIndices->push_back(idx);
	}
	numVisibleVisplanes = visibleVisplaneIndices->size();


	//Walls
	for (uint idx=0; idx<wallData->size(); idx++) {
		utils::Wall thisWall = wallData->at(idx);
		bool sProj = glm::dot(glm::vec2(thisWall.start - player->position), playerFDirection) < 0.0f;
		bool eProj = glm::dot(glm::vec2(thisWall.end - player->position), playerFDirection) < 0.0f;
		if (sProj && eProj) {continue; /* Completely behind player view */}

		visibleWallIndices->push_back(idx);
	}
	numVisibleWalls = visibleWallIndices->size();

}






void saveScreenshot(GLuint frameTextureID) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTextureID, 0);

	std::vector<unsigned char> pixels(currentRenderResolution.x * currentRenderResolution.y * 3);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, currentRenderResolution.x, currentRenderResolution.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	//Flip image vertically.
	for (int y = 0; y < currentRenderResolution.y / 2; ++y) {
		for (int x = 0; x < currentRenderResolution.x * 3; ++x) {
			std::swap(pixels[y * currentRenderResolution.x * 3 + x], pixels[(currentRenderResolution.y - 1 - y) * currentRenderResolution.x * 3 + x]);
		}
	}

	std::string timeStr = utils::getTimestamp();

	stbi_write_png(
		("screenshots/" + timeStr + ".png").c_str(),
		currentRenderResolution.x, currentRenderResolution.y,
		3, pixels.data(), currentRenderResolution.x*3
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::cout << "Successfully saved screenshot as : [" << timeStr << ".png]" << std::endl;
}



GLuint createGLImage2D(size_t width, size_t height, GLint internalFormat=GL_RGBA32F, GLint samplingType=GL_NEAREST, GLint edgeSampling=GL_REPEAT) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, samplingType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, samplingType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}


GLuint loadGLTexture2D(const std::string textureName, std::string subFolder="textures-env", int expectedWidth=-1, int expectedHeight=-1) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	int fallbackTextureWidth, fallbackTextureHeight, fallbackTextureChannels;
	unsigned char* fallbackTextureData = stbi_load(
		display::FALLBACK_TEXTURE_PATH,
		&fallbackTextureWidth, &fallbackTextureHeight,
		&fallbackTextureChannels, 4
	);

	if (!fallbackTextureData) {
		std::cerr << "Failed to load fallback texture : " << stbi_failure_reason() << std::endl;
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glDeleteTextures(1, &textureID);
		return 0;	
	}

	int width, height, channels;
	std::string texturePath = "src/" + subFolder + "/" + textureName + ".png";
	unsigned char* textureData = stbi_load(
		texturePath.c_str(),
		&width, &height,
		&channels, 4
	);

	if (!textureData) {
		//Try in folder beside stage XML with same name.
		texturePath = "stages/assets-" + stageData.name + "/" + textureName + ".png";
		textureData = stbi_load(
			texturePath.c_str(),
			&width, &height,
			&channels, 4
		);

		if (!textureData) {
			//Use fallback texture.
			textureData = fallbackTextureData;
			width = fallbackTextureWidth;
			height = fallbackTextureHeight;
			channels = fallbackTextureChannels;
		}
	}

	if ((expectedWidth != -1 && width != expectedWidth) || (expectedHeight != -1 && height != expectedHeight)) {
		std::cout << "Failed to load texture : " << textureName << ".png : Image was not correct resolution." << std::endl;
		std::cerr << "Expected [" << expectedWidth << ", " << expectedHeight << "] : Got [" << width << ", " << height << "]" << std::endl;
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &textureID);
		return 0;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(textureData);

	return textureID;
}



GLuint createTexture2DArray(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>& textureNames, std::string subFolder="textures-env", bool hasMipMap=false) {
	GLuint sheetArrayID;
	glGenTextures(1, &sheetArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, sheetArrayID);


	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, display::TEXTURE_RESOLUTION.x, display::TEXTURE_RESOLUTION.y, display::TEXTURE_ARRAY_MAX_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (hasMipMap) {
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_LOD, utils::configToInt("VIEW_TEXTURE_QUALITY"));
	} else {
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);



	int fallbackTextureWidth, fallbackTextureHeight, fallbackTextureChannels;
	bool usedFallback;

	unsigned char* fallbackTextureData = stbi_load(
		display::FALLBACK_TEXTURE_PATH,
		&fallbackTextureWidth, &fallbackTextureHeight,
		&fallbackTextureChannels, 4
	);

	if (!fallbackTextureData) {
		std::cerr << "Failed to load fallback texture : " << stbi_failure_reason() << std::endl;
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glDeleteTextures(1, &sheetArrayID);
		return 0;	
	}



	int width, height, channels;
	int layerIndex = 0;
	for (const std::string& textureName : textureNames) {
		if (textureName.empty()) continue;
		usedFallback = false;

		std::string reportedTextureName = textureName;
		std::string texturePath = "src/" + subFolder + "/" + textureName + ".png";
		unsigned char* textureData = stbi_load(
			texturePath.c_str(),
			&width, &height,
			&channels, 4
		);

		if (!textureData) {
			//Try in folder beside stage XML with same name.
			texturePath = "stages/assets-" + stageData.name + "/" + textureName + ".png";
			textureData = stbi_load(
				texturePath.c_str(),
				&width, &height,
				&channels, 4
			);

			if (!textureData) {
				//Use fallback texture.
				textureData = fallbackTextureData;
				width = fallbackTextureWidth;
				height = fallbackTextureHeight;
				channels = fallbackTextureChannels;
				reportedTextureName = "FALLBACK_TEXTURE";
				usedFallback = true;
			}
		}


		if (width != display::TEXTURE_RESOLUTION.x || height != display::TEXTURE_RESOLUTION.y) {
			std::cerr << "Texture " << reportedTextureName << " has incorrect dimensions (" << width << "x" << height << "). Expected "
					  << display::TEXTURE_RESOLUTION.x << "x" << display::TEXTURE_RESOLUTION.y << "." << std::endl;
			stbi_image_free(textureData);
			continue;
		}


		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, display::TEXTURE_RESOLUTION.x, display::TEXTURE_RESOLUTION.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, textureData);


		if (!usedFallback) {
			stbi_image_free(textureData);
		}

		layerIndex++;
		if (layerIndex >= display::TEXTURE_ARRAY_MAX_LAYERS) break;
	}

	if (hasMipMap) {glGenerateMipmap(GL_TEXTURE_2D_ARRAY);}
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	stbi_image_free(fallbackTextureData);

	return sheetArrayID;
}


GLuint createGLImage2DArray(size_t width, size_t height, size_t layers) {
	GLuint arrayID;
	glGenTextures(1, &arrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, arrayID);
	glTexStorage3D(
		GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F,
		width, height, layers
	);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);


	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	return arrayID;
}




GLuint getVAO() {
	const float vertices[] = {
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  //Bottom-left
		 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  //Bottom-right
		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  //Top-left
		 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  //Top-right
	};

	const int indices[] = {
		0, 1, 2,
		2, 3, 1,
	};

	// Create VAO (Vertex Array Object) to store all vertex state
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Create VBO (Vertex Buffer Object) to store vertex data
	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Fill the buffer with vertex data (positions + texture coordinates)
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Define the position attribute (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Define the texture coordinate attribute (location = 1)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0); // Unbind VAO

	return VAO;
}



float viewBob(float tick, utils::Player* player) {
	if (player->touchingFloor) {
		float seconds = tick / utils::configToFloat("VIEW_MAX_FREQ");
		float playerSpeed = length(glm::vec2(player->velocity.x, player->velocity.y));
		float speedMultiplier = glm::clamp(playerSpeed / playerConfig::MAX_AIR_SPEED_XY, 0.0f, 1.0f);
		float offset = sin(seconds * 6.0f) * 0.25f * speedMultiplier;
		return offset;
	}
	return 0.0f;
}


int tickCounter = 0, duration = 0;
glm::vec3 screenTintRGB = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec4 manageScreenTint(int newDuration=0, unsigned int event=E_NONE) {
	if (newDuration > 0) {
		tickCounter = newDuration;
		duration = newDuration;
		switch (event) {
			case E_NONE:
				screenTintRGB = glm::vec3(0.0f, 0.0f, 0.0f);
				break;
			case E_HURT:
				screenTintRGB = glm::vec3(1.0f, 0.0f, 0.0f);
				break;
			case E_HEAL:
				screenTintRGB = glm::vec3(0.0f, 1.0f, 0.0f);
				break;
			case E_ENERGY:
				screenTintRGB = glm::vec3(1.0f, 1.0f, 0.0f);
				break;
			case E_NEW_IH:
				screenTintRGB = glm::vec3(0.125f, 0.125f, 0.125f);
				break;
			default:
				screenTintRGB = glm::vec3(0.0f, 0.0f, 0.0f);
		}
	} else if (tickCounter != 0) {
		tickCounter--;
	}

	float intensity;
	if (duration > 0 && tickCounter > 0) {
		intensity = static_cast<float>(tickCounter) / static_cast<float>(duration);
	} else {
		intensity = 0;
	}
	return glm::vec4(screenTintRGB.x, screenTintRGB.y, screenTintRGB.z, intensity);
}








size_t currentDispVertexSize, currentDispIndexSize;
void createDispVAO(GLuint* VAO, GLuint* VBO, GLuint* EBO) {
	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);

	glGenBuffers(1, VBO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, constants::MAX_VERTEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentDispVertexSize = constants::MAX_VERTEX_BYTES;

	glGenBuffers(1, EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, constants::MAX_INDEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentDispIndexSize = constants::MAX_INDEX_BYTES;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
}


size_t currentUIVertexSize, currentUIIndexSize;
void createUIVAO(GLuint* VAO, GLuint* VBO, GLuint* EBO) {
	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);

	glGenBuffers(1, VBO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferData(GL_ARRAY_BUFFER, constants::MAX_VERTEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentUIVertexSize = constants::MAX_VERTEX_BYTES;

	glGenBuffers(1, EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, constants::MAX_INDEX_BYTES, nullptr, GL_DYNAMIC_DRAW); // Reserve space
	currentUIIndexSize = constants::MAX_INDEX_BYTES;

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void initialiseVAOs() {
	createDispVAO(&GLIndex::dispVAO, &GLIndex::dispVBO, &GLIndex::dispEBO);
	createUIVAO(&GLIndex::uiVAO, &GLIndex::uiVBO, &GLIndex::uiEBO);
}

GLuint displacementDrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
GLenum defaultBuffer = GL_BACK;
GLuint createDisplacementsFBO(size_t width, size_t height) {
	GLuint dispFBO;
	glGenFramebuffers(1, &dispFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, dispFBO);

	//Colour
	glGenTextures(1, &GLIndex::displacementFBOColour);
	glBindTexture(GL_TEXTURE_2D, GLIndex::displacementFBOColour);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GLIndex::displacementFBOColour, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//Position
	glGenTextures(1, &GLIndex::displacementFBOPosition);
	glBindTexture(GL_TEXTURE_2D, GLIndex::displacementFBOPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GLIndex::displacementFBOPosition, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//Normals
	glGenTextures(1, &GLIndex::displacementFBONormals);
	glBindTexture(GL_TEXTURE_2D, GLIndex::displacementFBONormals);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GLIndex::displacementFBONormals, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//Depth
	GLuint depthTex;
	glGenTextures(1, &depthTex);
	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	GLenum drawBuffers[] = { 
		GL_COLOR_ATTACHMENT0, 
		GL_COLOR_ATTACHMENT1, 
		GL_COLOR_ATTACHMENT2 
	};
	glDrawBuffers(3, drawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		raise("An error occurred while creating displacements FBO.");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return dispFBO;
}



glm::mat4 uiMatrix;
std::vector<utils::UIElement> UIElements;
void prepareOpenGL(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames, utils::Player* player) {
	//OpenGL setup;

	//Image2Ds
	GLIndex::renderedFrameID = createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLIndex::interfaceID = createGLImage2D(display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
	GLIndex::positionMapID = createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	GLIndex::normalMapID = createGLImage2D(currentShadowResolution.x, currentShadowResolution.y);
	GLIndex::lightingMapsArrayID = createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);

	//Textures
	GLIndex::textureArrayEnvironment = createTexture2DArray(*textureNames, "textures-env", true);
	GLIndex::textureArrayUI = createTexture2DArray(UIImageNames, "textures-sym");
	GLIndex::textureArrayNumeric = createTexture2DArray(symbolNames, "textures-sym");
	GLIndex::skyboxTextureID = loadGLTexture2D(stageData.skyboxTextureName, "textures-env", display::SKYBOX_RESOLUTION.x, display::SKYBOX_RESOLUTION.y);

	//FBO
	GLIndex::displacementFBO = createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);


	GLIndex::allVisplanesSSBO = createShaderStorageBufferObject(
		0, sizeof(utils::VisplaneGPU) * validVisplanes
	);
	GLIndex::allWallsSSBO = createShaderStorageBufferObject(
		1, sizeof(utils::WallGPU) * validWalls
	);
	GLIndex::spriteSSBO = createShaderStorageBufferObject(
		2, sizeof(utils::SpriteGPU) * validSprites
	);
	GLIndex::lightSSBO = createShaderStorageBufferObject(
		3, sizeof(utils::LightGPU) * validLights
	);
	GLIndex::displacementSSBO = createShaderStorageBufferObject(
		4, sizeof(utils::DisplacementGPU) * validDisplacements
	);
	GLIndex::visibleVisplaneIndicesSSBO = createShaderStorageBufferObject(
		5, sizeof(uint) * validVisplanes
	);
	GLIndex::visibleWallIndicesSSBO = createShaderStorageBufferObject(
		6, sizeof(uint) * validWalls
	);
	GLIndex::wallIntersectSSBO = createShaderStorageBufferObject(
		7, sizeof(utils::WallIntersect) * currentRenderResolution.x * validWalls
	);


	//Raycast compute shader
	GLIndex::raycastShader = createComputeShader("raycast");

	//Environment shader
	GLIndex::envShader = createShaderProgram("environment");

	//Displacement shaders
	GLIndex::displacementShader3D = createShaderProgram("displacements3D", "projection");
	GLIndex::displacementShader2D = createShaderProgram("displacements2D");

	//Sprite Shader
	GLIndex::spriteShader = createShaderProgram("sprites");

	//Lighting compute Shader
	GLIndex::lightingShader = createComputeShader("lighting");

	//uiShader
	GLIndex::uiShader = createShaderProgram("interface", "interface");
	
	//Display Shader
	GLIndex::displayShader = createShaderProgram("display", "display");


	initialiseVAOs();


	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	GLIndex::genericVAO = getVAO();

	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));
	uiMatrix = glm::ortho(0.0f, float(display::UI_RESOLUTION.x), 0.0f, float(display::UI_RESOLUTION.y), -1.0f, 1.0f);
	

	//Debug settings
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openGLErrorCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);



	UIElements = {
		UIElement(glm::vec2(-16, -72), glm::vec2(192, 192), static_cast<GLuint>(0)),	//Health image
		UIElement(glm::vec2(32, 32), glm::vec2(40, 40), &(player->health)),				//Health number
		UIElement(glm::vec2(780, -72), glm::vec2(192, 192), static_cast<GLuint>(1)), 	//Energy image
		UIElement(glm::vec2(840, 32), glm::vec2(40, 40), &(player->energy)),			//Energy number
		UIElement(glm::vec2(0, 508), glm::vec2(32, 32), &avgframerate, &shouldShowFPS),	//FPS number
		UIElement(glm::vec2(0, 476), glm::vec2(32, 32), &avgtickrate, &shouldShowTPS)	//TPS number
	};



	utils::GLErrorcheck("Initialisation", true); //Old basic debugging
}






}





namespace frame {


std::vector<uint> visibleVisplaneIndices;
std::vector<uint> visibleWallIndices;
void updateSSBOs(utils::DataSet* localGraphicsData, utils::Player* player) {
	visibleVisplaneIndices.clear();
	visibleWallIndices.clear();
	graphics::findVisibleObjects(player, &(localGraphicsData->visplaneData), &visibleVisplaneIndices, &(localGraphicsData->wallData), &visibleWallIndices);


	//Update SSBOs.
	graphics::updateShaderStorageBufferObject<utils::VisplaneGPU>(
		GLIndex::allVisplanesSSBO, player, &(localGraphicsData->visplaneData)
	);
	graphics::updateShaderStorageBufferObject<utils::WallGPU>(
		GLIndex::allWallsSSBO, player, &(localGraphicsData->wallData)
	);
	graphics::updateShaderStorageBufferObject<utils::DisplacementGPU>(
		GLIndex::displacementSSBO, player, &(localGraphicsData->displacementData)
	);
	graphics::updateShaderStorageBufferObject<utils::SpriteGPU>(
		GLIndex::spriteSSBO, player, &(localGraphicsData->spriteData)
	);
	graphics::updateShaderStorageBufferObject<utils::LightGPU>(
		GLIndex::lightSSBO, player, &(localGraphicsData->lightData)
	);
	graphics::updateShaderStorageBufferObject<uint>(
		GLIndex::visibleVisplaneIndicesSSBO, &visibleVisplaneIndices
	);
	graphics::updateShaderStorageBufferObject<uint>(
		GLIndex::visibleWallIndicesSSBO, &visibleWallIndices
	);
	utils::GLErrorcheck("Updating SSBOs", true);
}


void drawDisplacements(utils::DataSet* localGraphicsData, utils::Player* player) {
	std::vector<float> vertices;
	std::vector<GLuint> indices;
	std::vector<float> verticesData;
	std::vector<GLuint> indicesData;
	GLuint currentIdx = 0;

	int dispIdx = 0;
	for (utils::Displacement& thisDisp : localGraphicsData->displacementData) {
		glm::vec3 dispNormal = thisDisp.normal;
		verticesData = {
			thisDisp.vertices[0].x, thisDisp.vertices[0].y, thisDisp.vertices[0].z,
			thisDisp.UV[0].x, thisDisp.UV[0].y,	float(dispIdx),

			thisDisp.vertices[1].x, thisDisp.vertices[1].y, thisDisp.vertices[1].z,
			thisDisp.UV[1].x, thisDisp.UV[1].y,	float(dispIdx),

			thisDisp.vertices[2].x, thisDisp.vertices[2].y, thisDisp.vertices[2].z,
			thisDisp.UV[2].x, thisDisp.UV[2].y,	float(dispIdx),
		};
		indicesData = {
			currentIdx + 0, currentIdx + 1, currentIdx + 2
		};

		utils::combineVectors(&vertices, verticesData);
		utils::combineVectors(&indices, indicesData);

		currentIdx += 3; //3 vertices added.
		dispIdx++;
	}



	size_t newVertexSize = vertices.size() * sizeof(float);
	size_t newIndexSize = indices.size() * sizeof(GLuint);

	glBindFramebuffer(GL_FRAMEBUFFER, GLIndex::displacementFBO);
	glDrawBuffers(3, graphics::displacementDrawBuffers);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glUseProgram(GLIndex::displacementShader3D);
	glBindVertexArray(GLIndex::dispVAO);
	glBindBuffer(GL_ARRAY_BUFFER, GLIndex::dispVBO);
	if (newVertexSize > graphics::currentDispVertexSize) {
		glBufferData(GL_ARRAY_BUFFER, newVertexSize, vertices.data(), GL_DYNAMIC_DRAW);
		graphics::currentDispVertexSize = newVertexSize;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, newVertexSize, vertices.data());
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLIndex::dispEBO);
	if (newIndexSize > graphics::currentDispIndexSize) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, newIndexSize, indices.data(), GL_DYNAMIC_DRAW);
		graphics::currentDispIndexSize = newIndexSize;
	} else {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, newIndexSize, indices.data());
	}

	glBindTextureUnit(0, GLIndex::textureArrayEnvironment);

	uniforms::bindCommonUniforms(GLIndex::displacementShader3D, player);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDrawBuffers(1, &graphics::defaultBuffer);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	utils::GLErrorcheck("Displacements 3D", true);
}


//UI
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

void drawHUD(utils::DataSet* graphicsData, utils::Player* player) {
	currentIdx = 0;
	glClearTexImage(GLIndex::interfaceID, 0, GL_RGBA, GL_FLOAT, nullptr);
	std::vector<float> vertices;
	std::vector<GLuint> indices;
	std::vector<float> verticesData;
	std::vector<GLuint> indicesData;

	shouldShowFPS = utils::configToBool("META_SHOW_FRAMERATE_UI");
	shouldShowTPS = utils::configToBool("META_SHOW_TICKRATE_UI");


	for (const utils::UIElement element : graphics::UIElements) {
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


		float distance = glm::length(glm::vec2(thisTO.position - player->position));
		float scale = thisTO.scale * zoomEffect / distance;

		float centreX = utils::getCentreX(thisTO.position, player, display::UI_RESOLUTION);
		float projCentreY = (player->cameraPosition.z - thisTO.position.z) * zoomEffect / distance;
		float centreY = display::UI_RESOLUTION.y * (0.5f - projCentreY);
		float charY = centreY - (scale / 2.0f);
		float pitchDecimal = glm::clamp(player->viewPitch, -22.5f, 22.5f) * zoomEffect;
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
	glUseProgram(GLIndex::uiShader);
	glBindBuffer(GL_ARRAY_BUFFER, GLIndex::uiVBO);
	if (newVertexSize > graphics::currentUIVertexSize) {
		glBufferData(GL_ARRAY_BUFFER, newVertexSize, vertices.data(), GL_DYNAMIC_DRAW);
		graphics::currentUIVertexSize = newVertexSize;
	} else {
		glBufferSubData(GL_ARRAY_BUFFER, 0, newVertexSize, vertices.data());
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLIndex::uiEBO);
	if (newIndexSize > graphics::currentUIIndexSize) {
		glBufferData(GL_ARRAY_BUFFER, newIndexSize, vertices.data(), GL_DYNAMIC_DRAW);
		graphics::currentUIIndexSize = newIndexSize;
	} else {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, newIndexSize, indices.data());
	}

	glBindImageTexture(0, GLIndex::interfaceID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindTextureUnit(0, GLIndex::textureArrayUI);
	glBindTextureUnit(1, GLIndex::textureArrayNumeric);
	glBindTextureUnit(2, GLIndex::renderedFrameID);

	uniforms::bindCommonUniforms(GLIndex::uiShader, player);
	GLint pvmMatrixLocation = glGetUniformLocation(GLIndex::uiShader, "pvmMatrix");
	glUniformMatrix4fv(pvmMatrixLocation, 1, GL_FALSE, glm::value_ptr(graphics::uiMatrix));

	glBindVertexArray(GLIndex::uiVAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	utils::GLErrorcheck("Interface 3D", true);

}







inline void renderingGeneric(const std::string& shaderName="") {
	glBindVertexArray(GLIndex::genericVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	if (!shaderName.empty()) {
		utils::GLErrorcheck(shaderName, true);
	}
}


void draw(double blendingAlpha, utils::DataSet* localGraphicsData, utils::Player* player) {
	//Update resolution
	glViewport(0, 0, currentRenderResolution.x, currentRenderResolution.y);
	if (headLampEnabled) {
		lightFlickerRNG = utils::RNGc();
	}
	glm::vec4 tintData = graphics::manageScreenTint(0, player->state);
	float viewBob = (utils::configToBool("VIEW_BOB")) ? graphics::viewBob(tickNumber, player) : 0.0f;
	glm::vec3 interpPosition = glm::mix(player->prevPosition, player->position, blendingAlpha);
	player->cameraPosition = interpPosition + glm::vec3(0.0f, 0.0f, (player->height/3.0f) + viewBob);



	//Raycasting compute shader.
	const glm::uvec3 RAYCASTING_LOCAL_SIZE = glm::uvec3(32, 1, 1);
	glUseProgram(GLIndex::raycastShader);
	uniforms::bindCommonUniforms(GLIndex::raycastShader, player);
	glDispatchCompute(
		(currentRenderResolution.x + RAYCASTING_LOCAL_SIZE.x - 1) / RAYCASTING_LOCAL_SIZE.x,
		(numVisibleWalls + RAYCASTING_LOCAL_SIZE.y - 1) / RAYCASTING_LOCAL_SIZE.y,
		1
	);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLErrorcheck("Raycasting Shader", true);


	//Environment Shader.
	glUseProgram(GLIndex::envShader);
	glBindImageTexture(0, GLIndex::renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, GLIndex::positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, GLIndex::normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, GLIndex::textureArrayEnvironment);
	glBindTextureUnit(1, GLIndex::skyboxTextureID);

	//Uniforms
	uniforms::bindCommonUniforms(GLIndex::envShader, player);
	uniforms::bindUniformValue(GLIndex::envShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));
	uniforms::bindUniformValue(GLIndex::envShader, "allowTransparency", utils::configToBool("VIEW_ALLOW_TRANSPARENCY"));

	renderingGeneric("Environment Shader");

	
	//Displacements in 2 parts;
	//3D portion;
	frame::drawDisplacements(localGraphicsData, player);
	//2D portion;
	glUseProgram(GLIndex::displacementShader2D);
	glBindImageTexture(0, GLIndex::renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(1, GLIndex::positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, GLIndex::normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, GLIndex::renderedFrameID);
	glBindTextureUnit(1, GLIndex::displacementFBOColour);
	glBindTextureUnit(2, GLIndex::displacementFBOPosition);
	glBindTextureUnit(3, GLIndex::displacementFBONormals);

	//Uniforms
	uniforms::bindCommonUniforms(GLIndex::displacementShader2D, player);

	renderingGeneric("Environment Shader");


	//Sprite Shader.
	glUseProgram(GLIndex::spriteShader);
	glBindImageTexture(0, GLIndex::renderedFrameID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, GLIndex::positionMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindImageTexture(2, GLIndex::normalMapID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glBindTextureUnit(0, GLIndex::textureArrayEnvironment);

	//Uniforms
	uniforms::bindCommonUniforms(GLIndex::spriteShader, player);
	uniforms::bindUniformValue(GLIndex::spriteShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));

	renderingGeneric("Sprite Shader");



	//Lighting Shader
	const glm::uvec3 LIGHTING_LOCAL_SIZE = glm::uvec3(16, 16, 1);
	glUseProgram(GLIndex::lightingShader);

	glBindTextureUnit(0, GLIndex::positionMapID);
	glBindTextureUnit(1, GLIndex::normalMapID);
	glBindTextureUnit(2, GLIndex::textureArrayEnvironment);
	glBindImageTexture(0, GLIndex::lightingMapsArrayID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//Uniforms;
	uniforms::bindCommonUniforms(GLIndex::lightingShader, player);
	uniforms::bindUniformValue(GLIndex::lightingShader, "allowTransparency", utils::configToBool("VIEW_ALLOW_TRANSPARENCY") && utils::configToBool("VIEW_ALLOW_TRANSPARENT_SHADOWS"));
	uniforms::bindUniformValue(GLIndex::lightingShader, "useMipMapping", utils::configToBool("VIEW_MIPMAPPING"));
	uniforms::bindUniformValue(GLIndex::lightingShader, "headLampEnabled", headLampEnabled);
	uniforms::bindUniformValue(GLIndex::lightingShader, "headLampIntensity", 5.0f + (lightFlickerRNG / 768.0f)); //lightFlickerRNG is 0-255.

	//Dispatch 2 extra valid lights (Sun, Headlamp.)
	glDispatchCompute(
		(currentRenderResolution.x + LIGHTING_LOCAL_SIZE.x - 1) / LIGHTING_LOCAL_SIZE.x,
		(currentRenderResolution.y + LIGHTING_LOCAL_SIZE.y - 1) / LIGHTING_LOCAL_SIZE.y,
		(validLights + LIGHTING_LOCAL_SIZE.z + 1) / LIGHTING_LOCAL_SIZE.z
	);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLErrorcheck("Lighting Shader", true);



	//UI Shader.
	if (utils::configToBool("VIEW_SHOW_HUD")) {
		glViewport(0, 0, display::UI_RESOLUTION.x, display::UI_RESOLUTION.y);
		avgframerate = utils::getAverage(rollingFPS);
		avgtickrate = utils::getAverage(rollingTPS);
		drawHUD(localGraphicsData, player);
	}		


	//Display Shader and update screen.
	glViewport(0, 0, currentWindowResolution.x, currentWindowResolution.y);
	glUseProgram(GLIndex::displayShader);

	glBindTextureUnit(0, GLIndex::renderedFrameID);
	glBindTextureUnit(1, GLIndex::interfaceID);
	glBindTextureUnit(2, GLIndex::lightingMapsArrayID);
	glBindTextureUnit(3, GLIndex::normalMapID);
	glBindImageTexture(0, GLIndex::renderedFrameID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	//Uniforms
	uniforms::bindCommonUniforms(GLIndex::displayShader, player);
	//Display-Specific
	uniforms::bindUniformValue(GLIndex::displayShader, "antiAliasingLevel", utils::configToInt("VIEW_ANTIALIAS_LEVEL"));
	uniforms::bindUniformValue(GLIndex::displayShader, "smoothingEnabled", utils::configToBool("VIEW_SMOOTHING"));
	uniforms::bindUniformValue(GLIndex::displayShader, "quantisingLevel", utils::configToInt("VIEW_LUMINANCE_QUANTISATION"));
	uniforms::bindUniformValue(GLIndex::displayShader, "screenshotHasHUD", utils::configToBool("VIEW_INTERFACE_IN_SCREENSHOT"));

	renderingGeneric("Display Shader");


	if (shouldTakeScreenshot) {
		graphics::saveScreenshot(GLIndex::renderedFrameID);
	}
}


}
