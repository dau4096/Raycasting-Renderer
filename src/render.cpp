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


GLuint createShaderProgram(std::string name, bool hasVertexSource) {
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
		float zoomFactor;
		float maxRayAngle;
		float maxRayDistance;

		float topIndex;
		float lowIndex;

		glm::vec2 textureSize;

		float drawUV;

		float padding[2];
	};

	ConstData constData = {
		display::zoomFactor,
		display::maxRayAngle,
		display::maxRayDistance,

		display::topIndex,
		display::lowIndex,

		{constants::textureWidth, constants::textureHeight},

		static_cast<float>(dev::drawUV),

		{0.0f, 0.0f},
	};

	GLuint constUBO;
	glGenBuffers(1, &constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, constUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(ConstData), &constData, GL_STATIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, constUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



void createWallUBO(const std::array<utils::Wall, 128>* dataSet) {
	GLuint wallUBO;
	glGenBuffers(1, &wallUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, wallUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Wall) * dataSet->size(), dataSet->data(), GL_STATIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, wallUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


GLuint createSpriteUBO() {
	GLuint spriteUBO;
	glGenBuffers(1, &spriteUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, spriteUBO);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(utils::Sprite) * 32, nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, spriteUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	return spriteUBO;
}


void updateSpriteUBO(GLuint* spriteUBO, const std::array<utils::Sprite, 32>* dataSet) {
	glBindBuffer(GL_UNIFORM_BUFFER, *spriteUBO);
	void* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	
	if (ptr) {
		memcpy(ptr, dataSet->data(), sizeof(utils::Sprite) * dataSet->size());
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	} else {
		raise("Failed to write data to spriteUBO.");
	}

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
    const int maxTextureArrayLayers = 32;

    // Texture array ID
    GLuint sheetArrayID;
    glGenTextures(1, &sheetArrayID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, sheetArrayID);

    // Allocate storage for the texture array
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, constants::textureWidth, constants::textureHeight, maxTextureArrayLayers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load textures and populate the array
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

        // Ensure all textures match the expected dimensions
        if (width != constants::textureWidth || height != constants::textureHeight) {
            std::cerr << "Texture " << textureName << " has incorrect dimensions (" << width << "x" << height << "). Expected "
                      << constants::textureWidth << "x" << constants::textureHeight << "." << std::endl;
            stbi_image_free(textureData);
            continue;
        }

        // Upload texture to the correct layer of the 2D array
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, constants::textureWidth, constants::textureHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

        // Free the texture data after uploading
        stbi_image_free(textureData);

        layerIndex++;
        if (layerIndex >= maxTextureArrayLayers) break; // Prevent exceeding maximum layers
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0); // Unbind texture array

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