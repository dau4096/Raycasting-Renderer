#include "includes.h"
#include "utils.h"
#include "C:/Users/User/Documents/code/.cpp/stb_image.h"
using namespace std;
using namespace utils;
using namespace glm;


namespace render {
//Functions



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


std::string readFile(const std::string& filePath) {
	std::ifstream fileStream(filePath);
	if (!fileStream.is_open()) {
		raise("Error: Could not open file: " + string(filePath));
		return "";
	}

	std::stringstream buffer;
	buffer << fileStream.rdbuf();
	return buffer.str();
}


GLuint compileShader(GLenum shaderType, string filePath) {
	std::string source = readFile(filePath);
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
		char infolog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infolog);
		raise("Error: Program linking failed;\n" + string(infolog));
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}


void createConstUBO() {
	struct ConstData {
		float ZOOM_MULT;
		float MAX_RAY_ANGLE;
		float MAX_RAY_DIST;

		glm::vec2 TEXTURE_RESOLUTION;
	};

	ConstData constData = {
		display::ZOOM_MULT,
		display::MAX_RAY_ANGLE,
		display::MAX_RAY_DIST,

		{constants::TEXTURE_RESOLUTION.x, constants::TEXTURE_RESOLUTION.y}
	};

	GLuint constUBO;
	glGenBuffers(1, &constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, constUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(ConstData), &constData, GL_STATIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 10, constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


GLuint createVisplaneUBO() {
	GLuint visplaneUBO;
	glGenBuffers(1, &visplaneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, visplaneUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Visplane) * constants::MAX_VISPLANES, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 7, visplaneUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return visplaneUBO;
}

void updateVisplaneUBO(GLuint visplaneUBO, std::array<utils::Visplane, constants::MAX_VISPLANES>* dataSet) {
    glBindBuffer(GL_UNIFORM_BUFFER, visplaneUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(utils::Visplane) * constants::MAX_VISPLANES, dataSet->data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createWallUBO() {
	GLuint wallUBO;
	glGenBuffers(1, &wallUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, wallUBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(utils::Wall) * constants::MAX_WALLS, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, wallUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return wallUBO;
}

void updateWallUBO(GLuint wallUBO, std::array<utils::Wall, constants::MAX_WALLS>* dataSet) {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, wallUBO);
	void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Wall) * constants::MAX_WALLS);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	} else {
		raise("Failed to write data to wallUBO.");
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}



GLuint createSpriteSSBO() {
	GLuint spriteSSBO;
	glGenBuffers(1, &spriteSSBO);
	glBindBuffer(GL_UNIFORM_BUFFER, spriteSSBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Sprite) * constants::MAX_SPRITES, nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 4, spriteSSBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return spriteSSBO;
}

void updateSpriteSSBO(GLuint spriteSSBO, std::array<utils::Sprite, constants::MAX_SPRITES>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, spriteSSBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Sprite) * constants::MAX_SPRITES);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to spriteUBO.");
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createLightSSBO() {
	GLuint lightSSBO;
	glGenBuffers(1, &lightSSBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightSSBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Light) * constants::MAX_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 5, lightSSBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return lightSSBO;
}

void updateLightSSBO(GLuint lightSSBO, std::array<utils::Light, constants::MAX_LIGHTS>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, lightSSBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Light) * constants::MAX_LIGHTS);
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to lightUBO.");
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createDepthSSBO(int width) {
	GLuint SSBO;

	glGenBuffers(1, &SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);

	glBufferData(GL_SHADER_STORAGE_BUFFER, width * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, SSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return SSBO;
}



GLuint createTexture(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}


GLuint createTextureArray(const std::array<std::string, 32>& textureNames) {
	GLuint sheetArrayID;
	glGenTextures(1, &sheetArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, sheetArrayID);


	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, constants::TEXTURE_RESOLUTION.x, constants::TEXTURE_RESOLUTION.y, constants::TEXTURE_ARRAY_MAX_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);


	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	int width, height, channels;
	int layerIndex = 0;

	for (const std::string& textureName : textureNames) {
		if (textureName.empty()) continue;

		std::string texturePath = "src/textures/" + textureName + ".bmp";
		unsigned char* textureData = stbi_load(texturePath.c_str(), &width, &height, &channels, 4); // Force RGBA (4 channels)

		if (!textureData) {
			std::cerr << "Failed to load image " << texturePath << ": " << stbi_failure_reason() << std::endl;
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			glDeleteTextures(1, &sheetArrayID);
			return 0; // Indicate failure
		}


		if (width != constants::TEXTURE_RESOLUTION.x || height != constants::TEXTURE_RESOLUTION.y) {
			std::cerr << "Texture " << textureName << " has incorrect dimensions (" << width << "x" << height << "). Expected "
					  << constants::TEXTURE_RESOLUTION.x << "x" << constants::TEXTURE_RESOLUTION.y << "." << std::endl;
			stbi_image_free(textureData);
			continue;
		}


		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, constants::TEXTURE_RESOLUTION.x, constants::TEXTURE_RESOLUTION.y, 1, GL_RGBA, GL_UNSIGNED_BYTE, textureData);


		stbi_image_free(textureData);

		layerIndex++;
		if (layerIndex >= constants::TEXTURE_ARRAY_MAX_LAYERS) break;
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

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

}