#include "includes.h"
#include "utils.h"
using namespace std;
using namespace glm;


namespace utils {

void print(std::string value) {
	std::cout << value << std::endl;
}


void printVec2(glm::vec2 vector) {
	std::cout << "<" << vector.x << ", " << vector.y << ">" << std::endl;
}


void raise(string err) {
	std::cerr << err << std::endl;
	std::string end;
	std::cin >> end;
}

void pause() {
	string pause;
	std::cin >> pause;
}

void GLErrorcheck(std::string location, bool shouldPause) {
	GLenum GLError;
	GLError = glGetError();
	if (GLError != GL_NO_ERROR) {
		std::cerr << location << " | OpenGL error; " << GLError << std::endl;
		if (shouldPause) {pause();}
	}
}


float determinant(glm::vec2 vecA, glm::vec2 vecB) {
	return vecA.x * vecB.y - vecA.y * vecB.x;
}


float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return fmod(value, 360.0f);
}


// FrameBuffer Class method implementations
FrameBuffer::FrameBuffer(int width, int height) : width(width), height(height) {
	data.resize(width * height * 3);
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int i = (y * width + x) * 3;
			glm::vec3 colour = (y > height / 2) ? display::topColour : display::lowColour;
			setPixel(i, colour);
		}
	}
}

unsigned char* FrameBuffer::operator[](int y) {
	return &data[y * width * 3]; // Return a pointer to the start of the row
}

int FrameBuffer::getWidth() {return width;};
int FrameBuffer::getHeight() {return height;};

unsigned char* FrameBuffer::getData() {
	return data.data(); // Return a pointer to the raw data
}

void FrameBuffer::clearBuffer() {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int i = (y * width + x) * 3;
			glm::vec3 colour = (y > height / 2) ? display::topColour : display::lowColour;
			setPixel(i, colour);
		}
	}
}


void FrameBuffer::drawLine(int xCoord, int lineHeight, utils::Wall wall, glm::vec2 position, unsigned char* textureData, float multiplier) {
	if (xCoord < 0 || xCoord >= width || lineHeight < 1) {
		return;
	}

	int midPointY = this->height / 2;

	float repeatInterval = 1.0f;

	glm::vec2 wallDirection = wall.end - wall.start;
	glm::vec2 wallPosition = position - wall.start;
	float wallLength = glm::length(wallDirection);
	float projection = glm::dot(wallPosition, glm::normalize(wallDirection));
	float xUV = fmod(projection / repeatInterval, 1.0f);
	if (xUV < 0.0f) xUV += 1.0f;


	for (int yOffset = -lineHeight / 2; yOffset <= lineHeight / 2; yOffset++) {
		int yCoord = midPointY + yOffset;
		if (yCoord < 0 || yCoord >= height) {
			continue;
		}


		float yUV = (yCoord - (midPointY - lineHeight / 2.0f)) / lineHeight;
		yUV = glm::clamp(yUV, 0.0f, 1.0f);

		glm::vec3 pixelColour;
		if (dev::drawUV) {
			pixelColour = glm::vec3(xUV*255, yUV*255, 0.0f);
		} else {
			pixelColour = getPixelData(xUV, yUV, textureData) * multiplier;
		}

		setPixel(getIndex(xCoord, yCoord), pixelColour);
	}
}


void FrameBuffer::setPixel(int index, glm::vec3 colour) {
	data[index + 0] = colour.x;
	data[index + 1] = colour.y;
	data[index + 2] = colour.z;	
}

int FrameBuffer::getIndex(int xCoord, int yCoord) {
	return 3 * (xCoord + yCoord * width);
}




void printFramebuffer(FrameBuffer frameBuffer) {
	int width = frameBuffer.getWidth();
	int height = frameBuffer.getHeight();
	unsigned char* data = frameBuffer.getData();

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index = (y * width + x) * 3;
			unsigned char r = data[index];	 // Red component
			unsigned char g = data[index + 1]; // Green component
			unsigned char b = data[index + 2]; // Blue component
			
			// Print each pixel in the format (R, G, B)
			cout << "(" 
					  << hex << static_cast<int>(r) << "," 
					  << hex << static_cast<int>(g) << "," 
					  << hex << static_cast<int>(b) << ") ";
		}
		cout << endl;
	}
}


bool saveTextureToFile(GLuint textureID, int width, int height, const std::string& filename) {
	return true;

	/*
	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Allocate a buffer to store the texture data (RGBA format)
	std::vector<unsigned char> buffer(width * height * 3);

	// Read the texture data
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());

	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// Check for OpenGL errors
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cerr << "OpenGL Error while reading texture: " << err << std::endl;
		return false;
	}

	// Use stb_image_write to save the data as a PNG file
	*/
	/*
	if (stbi_write_png(filename.c_str(), width, height, 3, buffer.data(), width * 3)) {
		std::cout << "Texture saved to " << filename << std::endl;
		return true;
	} else {
		std::cerr << "Failed to save texture to " << filename << std::endl;
		return false;
	}
	*/
	/*
	
	for (unsigned char c : buffer) {
		std::cout << static_cast<int>(c) << " ";
	}
	raise("Saved Image");
	*/
}


glm::vec3 getPixelData(float xUV, float yUV, unsigned char* textureData) {
	int texX = static_cast<int>(xUV * (constants::textureWidth - 1));
	int texY = static_cast<int>((1.0f - yUV) * (constants::textureHeight - 1));
	int pixelIndex = (texY * constants::textureWidth + texX) * 3;
	int red = textureData[pixelIndex];
	int green = textureData[pixelIndex + 1];
	int blue = textureData[pixelIndex + 2];
	
	return glm::vec3(red, green, blue);
}

}