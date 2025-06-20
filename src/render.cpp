#include "includes.h"
#include "global.h"
#include "utils.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image_write.h"
using namespace std;
using namespace utils;
using namespace glm;


namespace render {

GLFWwindow* initializeWindow(int width, int height, const char* title) {
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



GLuint compileShader(GLenum shaderType, string filePath) {
	std::string source = utils::readFile(filePath);
	const char* src = source.c_str();

	// Create a shader object
	GLuint shader = glCreateShader(shaderType);
	if (shader == 0) {
		raise("Error: Failed to create shader.");
		return 0;
	}

	// Attach the shader source code to the shader object
	glShaderSource(shader, 1, &src, nullptr);

	// Compile the shader
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


GLuint createShaderProgram(std::string name, bool hasVertexSource=true) {
	GLuint vertexShader;
	if (hasVertexSource) {
		vertexShader = compileShader(GL_VERTEX_SHADER, "src\\shaders\\"+ name +".vert");
	} else {
		vertexShader = compileShader(GL_VERTEX_SHADER, "src\\shaders\\generic.vert");
	}
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "src\\shaders\\"+ name +".frag");

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





void saveScreenshot(GLuint frameTextureID) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTextureID, 0);

	std::vector<unsigned char> pixels(currentWindowResolution.x * currentWindowResolution.y * 3);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, currentWindowResolution.x, currentWindowResolution.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	//Flip image vertically.
	for (int y = 0; y < currentWindowResolution.y / 2; ++y) {
		for (int x = 0; x < currentWindowResolution.x * 3; ++x) {
			std::swap(pixels[y * currentWindowResolution.x * 3 + x], pixels[(currentWindowResolution.y - 1 - y) * currentWindowResolution.x * 3 + x]);
		}
	}

	std::string timeStr = utils::getTimestamp();

	stbi_write_png(
		("screenshots/" + timeStr + ".png").c_str(),
		currentWindowResolution.x, currentWindowResolution.y,
		3, pixels.data(), currentWindowResolution.x*3
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::cout << "Successfully saved screenshot as : [" << timeStr << ".png]" << std::endl;
}



GLuint createGLImage2D(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

	//Create VAO
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Create VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Create EBO
	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//Define position attr
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Define UV attr
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0); // Unbind VAO

	return VAO;
}


template<typename T>
static inline void combineVectors(std::vector<T>* A, std::vector<T>& B) {
	A->insert(A->end(), B.begin(), B.end());
}

GLuint createVAO(
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Displacement>* displacementData,
		std::vector<utils::Sprite>* spriteData,
		std::vector<utils::TextObject>* textObjectData,
		size_t* numTris, utils::Player* player
	) {
	std::vector<float> vertices;
	std::vector<GLuint> indices;
	GLuint currentIndex = 0;


	for (size_t idx=0; idx<validVisplanes; idx++) {
		utils::Visplane thisVisplane = visplaneData->at(idx);
		std::vector<float> vertexData = {
			thisVisplane.start.x, 	thisVisplane.start.y,	thisVisplane.height, 	-1.0f, -1.0f, static_cast<float>(thisVisplane.textureID), 1.0f,
			thisVisplane.end.x, 	thisVisplane.start.y,	thisVisplane.height, 	-1.0f, -1.0f, static_cast<float>(thisVisplane.textureID), 1.0f,
			thisVisplane.end.x, 	thisVisplane.end.y,		thisVisplane.height, 	-1.0f, -1.0f, static_cast<float>(thisVisplane.textureID), 1.0f,
			thisVisplane.start.x, 	thisVisplane.end.y,		thisVisplane.height, 	-1.0f, -1.0f, static_cast<float>(thisVisplane.textureID), 1.0f,
		};
		std::vector<GLuint> indexData = {
			currentIndex, currentIndex + 1, currentIndex + 2,
			currentIndex, currentIndex + 2, currentIndex + 3
		};

		currentIndex += 4; //4 vertices for Visplanes

		combineVectors(&vertices, vertexData);
		combineVectors(&indices, indexData);
	}


	for (size_t idx=0; idx<validWalls; idx++) {
		utils::Wall thisWall = wallData->at(idx);
		float TYPEIDX = (abs(thisWall.direction.x) > abs(thisWall.direction.y)) ? 2.0f : 3.0f;
		std::vector<float> vertexData = {
			thisWall.start.x, 	thisWall.start.y,	thisWall.start.z, 	-1.0f, -1.0f, static_cast<float>(thisWall.textureID), TYPEIDX,
			thisWall.end.x, 	thisWall.end.y,		thisWall.start.z, 	-1.0f, -1.0f, static_cast<float>(thisWall.textureID), TYPEIDX,
			thisWall.end.x, 	thisWall.end.y,		thisWall.end.z,		-1.0f, -1.0f, static_cast<float>(thisWall.textureID), TYPEIDX,
			thisWall.start.x, 	thisWall.start.y,	thisWall.end.z, 	-1.0f, -1.0f, static_cast<float>(thisWall.textureID), TYPEIDX,
		};
		std::vector<GLuint> indexData = {
			currentIndex, currentIndex + 1, currentIndex + 2,
			currentIndex, currentIndex + 2, currentIndex + 3
		};

		currentIndex += 4; //4 vertices for Walls

		combineVectors(&vertices, vertexData);
		combineVectors(&indices, indexData);
	}


	for (size_t idx=0; idx<validDisplacements; idx++) {
		utils::Displacement thisDisp = displacementData->at(idx);
		std::vector<float> vertexData = {
			thisDisp.vertices.at(0).x, thisDisp.vertices.at(0).y, thisDisp.vertices.at(0).z, thisDisp.UV.at(0).x, thisDisp.UV.at(0).y, static_cast<float>(thisDisp.textureID), 4.0f,
			thisDisp.vertices.at(1).x, thisDisp.vertices.at(1).y, thisDisp.vertices.at(1).z, thisDisp.UV.at(1).x, thisDisp.UV.at(1).y, static_cast<float>(thisDisp.textureID), 4.0f,
			thisDisp.vertices.at(2).x, thisDisp.vertices.at(2).y, thisDisp.vertices.at(2).z, thisDisp.UV.at(2).x, thisDisp.UV.at(2).y, static_cast<float>(thisDisp.textureID), 4.0f,
		};
		std::vector<GLuint> indexData = {
			currentIndex, currentIndex + 1, currentIndex + 2
		};

		currentIndex += 3; //3 Vertices for Displacements

		combineVectors(&vertices, vertexData);
		combineVectors(&indices, indexData);
	}


	float angle = (player->viewAngle + 90.0f) * constants::TO_RAD;
	glm::vec3 right = glm::vec3(sin(angle), cos(angle), 0.0f);
	for (size_t idx=0; idx<validSprites; idx++) {
		utils::Sprite thisSprite = spriteData->at(idx);
		glm::vec3 leftPos = thisSprite.position - right;
		glm::vec3 rightPos = thisSprite.position + right;
		float lowZ = thisSprite.position.z - (thisSprite.height / 2.0f);
		float highZ = thisSprite.position.z + (thisSprite.height / 2.0f);

		std::vector<float> vertexData = {
			leftPos.x, 		leftPos.y,		lowZ, 	0.0f, 0.0f, static_cast<float>(thisSprite.textureID), 5.0f,
			rightPos.x, 	rightPos.y,		lowZ, 	1.0f, 0.0f, static_cast<float>(thisSprite.textureID), 5.0f,
			rightPos.x, 	rightPos.y,		highZ,	1.0f, 1.0f, static_cast<float>(thisSprite.textureID), 5.0f,
			leftPos.x, 		leftPos.y,		highZ, 	0.0f, 1.0f, static_cast<float>(thisSprite.textureID), 5.0f,
		};
		std::vector<GLuint> indexData = {
			currentIndex, currentIndex + 1, currentIndex + 2,
			currentIndex, currentIndex + 2, currentIndex + 3
		};

		currentIndex += 4; //4 Vertices for Sprites

		combineVectors(&vertices, vertexData);
		combineVectors(&indices, indexData);
	}


	*numTris = indices.size();


	//Create VAO
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Create VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	//Create EBO
	GLuint EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	//Define position attr
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Define UV attr
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//Define UV attr
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	if (dev::SHOW_BUFFER_SIZES) {
		std::cout << "Created VAO with " << vertices.size()/7 << " vertices and " << indices.size() << " indices" << std::endl;
	}
	return VAO;
}






float viewBob(float tick, utils::Player player) {
	if (player.touchingFloor) {
		float seconds = tick / utils::configToFloat("VIEW_MAX_FREQ");
		float playerSpeed = length(glm::vec2(player.velocity.x, player.velocity.y));
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

}