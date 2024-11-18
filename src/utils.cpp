#include "includes.h"
#include "utils.h"
using namespace std;
using namespace glm;


namespace utils {

#define print(value) std::cout << value << std::endl;


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


// FrameBuffer Class method implementations
FrameBuffer::FrameBuffer(int width, int height) : width(width), height(height) {
	data.resize(width * height * 3); // 3 channels for RGB
	for (int i = 0; i < width * height * 3; i += 3) {
		bool topHalf = i >= width*height*1.5;

		data[i + 0] = (topHalf) ? display::topColour[0] : display::lowColour[0];
		data[i + 1] = (topHalf) ? display::topColour[1] : display::lowColour[1];
		data[i + 2] = (topHalf) ? display::topColour[2] : display::lowColour[2];
	}
};

unsigned char* FrameBuffer::operator[](int y) {
	return &data[y * width * 3]; // Return a pointer to the start of the row
}

int FrameBuffer::getWidth() {return width;};
int FrameBuffer::getHeight() {return height;};

unsigned char* FrameBuffer::getData() {
	return data.data(); // Return a pointer to the raw data
}

void FrameBuffer::clearBuffer() {
	for (int i = 0; i < width * height * 3; i += 3) {
		bool topHalf = i >= width*height*1.5;

		data[i + 0] = (topHalf) ? display::topColour[0] : display::lowColour[0];
		data[i + 1] = (topHalf) ? display::topColour[1] : display::lowColour[1];
		data[i + 2] = (topHalf) ? display::topColour[2] : display::lowColour[2];
	}	
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
	/*
	if (stbi_write_png(filename.c_str(), width, height, 3, buffer.data(), width * 3)) {
		std::cout << "Texture saved to " << filename << std::endl;
		return true;
	} else {
		std::cerr << "Failed to save texture to " << filename << std::endl;
		return false;
	}
	*/
	
    for (unsigned char c : buffer) {
        std::cout << static_cast<int>(c) << " ";
    }
	raise("done");
}

}