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
		alignas(4) float ZOOM_MULT;
		alignas(4) float MAX_RAY_ANGLE;
		alignas(4) float MAX_RAY_DIST;

		alignas(8) glm::vec2 TEXTURE_RESOLUTION;

		alignas(16) float _padding[4];
	};

	ConstData constData = {
		display::ZOOM_MULT,
		display::MAX_RAY_ANGLE,
		display::MAX_RAY_DIST,

		{constants::TEXTURE_RESOLUTION.x, constants::TEXTURE_RESOLUTION.y},

		{0.0f, 0.0f, 0.0f, 0.0f},
	};

	GLuint constUBO;
	glGenBuffers(1, &constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, constUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(ConstData), &constData, GL_STATIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


GLuint createVisplaneUBO() {
	GLuint visplaneUBO;
	glGenBuffers(1, &visplaneUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visplaneUBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(utils::Visplane) * 64, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, visplaneUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return visplaneUBO;
}

void updateVisplaneUBO(GLuint visplaneUBO, std::vector<utils::Visplane>* dataSet) {
	//Debugging contents; looks like a spoiled mess.
	for (uint i=0; i<64; i++) {
		utils::Visplane plane = (*dataSet)[i];
		cout << plane.start.x << "," << plane.start.y << " / "<< plane.end.x << "," << plane.end.y << " / " << plane.height << " / " << plane.textureID << " / " << plane.valid << endl;
	}
	//Excerpt;
	/*
	-10,-10 / 10,10 / -1 / 2 / 1
	0,0 / 34761.6,6.38992e-43 / 34765.3 / 456 / 1191693520
	3.22299e-44,0 / 3.78351e-44,0 / 9.82125e-39 / -1879036416 / 7077986
	8.90819e-39,9.27552e-39 / 9.27553e-39,1.01939e-38 / 9.18369e-39 / 115 / 0
	34756.6,6.38992e-43 / 34490,6.38992e-43 / 0 / 0 / 15
	5.1848e-44,0 / 1.4013e-45,0 / 2.1971e+15 / 32765 / 1191694176
	0,0 / 0,0 / 2.84177e-39 / -1879035648 / 1
	34760.9,6.38992e-43 / 0,0 / 0 / 0 / 0
	34713.1,6.38992e-43 / 34549.2,6.38992e-43 / 0 / 0 / 32
	4.34403e-44,0 / 1.4013e-45,0 / 2.19754e+15 / 32765 / 1191891536
	0,0 / 0,0 / 2.69856e-40 / -1879034880 / 7143529
	1.04694e-38,1.05612e-38 / 9.27555e-39,1.59748e-43 / 0 / 0 / 0
	3.56709e+12,4.59135e-41 / 0,1.4013e-45 / 0 / 1 / 1414534912
	0,0 / 34768.6,6.38992e-43 / 1.43867e+38 / 0 / 0
	0,0 / 1.43867e+38,0 / 5.04532e-39 / -2013251840 / 5505102
	9.64289e-39,1.11123e-38 / 1.01939e-38,9.82656e-39 / 9.2755e-39 / 7733362 / 6488169
	4.25995e-43,0 / 3.67413e-40,2.8026e-45 / 4.2039e-43 / 0 / 1543109224
	0,0 / nan,nan / nan / 0 / 0
	9.4062e-38,0 / 0,0 / 5.41268e-39 / -1879033344 / 6619236
	8.9082e-39,9.64286e-39 / 8.90818e-39,9.27554e-39 / 8.9082e-39 / 0 / 0
	7.71429e-39,5.96935e-39 / 6.70411e-39,8.17348e-39 / 6.97963e-39 / 4390991 / 4980801
	6.33674e-39,0 / 34712.9,6.38992e-43 / 34762.6 / 456 / 1191665552
	3.36312e-44,0 / 4.90454e-44,0 / 4.31067e-39 / -2013250304 / 1191691600
	34759.3,6.38992e-43 / 2.52234e-44,0 / 3.22299e-44 / 0 / 25
	0,0 / 0,0 / 0 / 0 / 0
	*/
	//Probably misaligned bits somewhere.



	glBindBuffer(GL_UNIFORM_BUFFER, visplaneUBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Visplane) * dataSet->size());
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to visplaneUBO.");
	}

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createWallUBO() {
	GLuint wallUBO;
	glGenBuffers(1, &wallUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, wallUBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(utils::Wall) * 256, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, wallUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return wallUBO;
}

void updateWallUBO(GLuint wallUBO, std::vector<utils::Wall>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, wallUBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Wall) * dataSet->size());
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to wallUBO.");
	}

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createSpriteUBO() {
	GLuint spriteUBO;
	glGenBuffers(1, &spriteUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, spriteUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Sprite) * 32, nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 4, spriteUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return spriteUBO;
}

void updateSpriteUBO(GLuint spriteUBO, std::vector<utils::Sprite>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, spriteUBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Sprite) * dataSet->size());
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to spriteUBO.");
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



GLuint createLightUBO() {
	GLuint lightUBO;
	glGenBuffers(1, &lightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Light) * 64, nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 5, lightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return lightUBO;
}

void updateLightUBO(GLuint lightUBO, std::vector<utils::Light>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Light) * dataSet->size());
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

	// Allocate storage for the texture with a suitable format
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);

	// Set filtering (optional, doesn't matter much for image load/store)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}


GLuint createTextureArray(const std::array<std::string, 32>& textureNames) {
	GLuint sheetArrayID;
	glGenTextures(1, &sheetArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, sheetArrayID);


	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, constants::TEXTURE_RESOLUTION.x, constants::TEXTURE_RESOLUTION.y, constants::TEXTURE_ARRAY_MAX_LAYERS, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);


	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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