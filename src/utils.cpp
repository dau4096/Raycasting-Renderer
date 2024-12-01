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


void printVec3(glm::vec3 vector) {
	std::cout << "<" << vector.x << ", " << vector.y << ", " << vector.z << ">" << std::endl;
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
	depths.resize(width*height, display::maxRayDistance);
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int i = (y * width + x) * 3;
			glm::vec3 colour = (y > height / 2) ? display::topColour : display::lowColour;
			setPixel(i, colour);
		}
	}
}

unsigned char* FrameBuffer::operator[](int y) {
	if (y < 0 || y >= height) {return nullptr;}
	return &data[y * width * 3]; // Return a pointer to the start of the row
}

int FrameBuffer::getWidth() {return width;};
int FrameBuffer::getHeight() {return height;};

void FrameBuffer::setDepth(int index, float depth) {
	if (index < 0 || index >= this->width * this->height) {return;}
	depths[index] = depth;
}
float FrameBuffer::getDepth(int index) {
	if (index < 0 || index >= this->width * this->height) {return display::maxRayDistance;}
	return depths[index];
}
unsigned char* FrameBuffer::getData() {
	return data.data(); // Return a pointer to the raw data
}

void FrameBuffer::clearBuffer() {
	std::fill(depths.begin(), depths.end(), display::maxRayDistance);
    for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int i = 3*(y * width + x);
			glm::vec3 colour = (y > height / 2) ? display::topColour : display::lowColour;
			setPixel(i, colour);
		}
	}
}


void FrameBuffer::drawWallLine(int xCoord, int lineHeight, utils::Wall wall, glm::vec2 position, float depth, utils::Texture texture, float multiplier) {
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
			pixelColour = getPixelData(xUV, yUV, texture) * multiplier;
		}

		if (pixelColour == glm::vec3(-1.0f)) {
			continue; //Pixel is transparent.
		}

		int pixelIndex = getIndex(xCoord, yCoord);
		setDepth(pixelIndex, depth);
		setPixel(pixelIndex*3, pixelColour);
	}
}


void FrameBuffer::drawSpriteLine(int xCoord, int lineHeight, utils::Sprite sprite, int spriteX, float spriteWidth, float depth, utils::Texture texture) {
	if (xCoord < 0 || xCoord >= width || lineHeight < 1) {
		return;
	}


	int midPointY = this->height/2;
	float xUV = spriteX / spriteWidth;

	float distanceMultiplier = 1.0f - (2.0f * depth) / display::maxRayDistance;


	for (int yOffset = -lineHeight/2; yOffset <= lineHeight/2; yOffset++) {
		int yCoord = midPointY + yOffset;
		if (yCoord < 0 || yCoord > this->height) {
			continue;
		}

		float yUV = (yCoord - (midPointY - lineHeight / 2.0f)) / lineHeight;
		yUV = glm::clamp(yUV, 0.0f, 1.0f);



		int pixelIndex = getIndex(xCoord, yCoord);
		float savedDepth = getDepth(pixelIndex);
		if (savedDepth < depth) {continue;} //Pixel is covered.

		glm::vec3 pixelColour;
		if (dev::drawUV) {
			pixelColour = glm::vec3(255.0f, 0.0f, 255.0f);
		} else {
			pixelColour = getPixelData(xUV, yUV, texture) * distanceMultiplier;
		}

		if (pixelColour == glm::vec3(-1.0f)) {continue;} //Pixel is transparent.


		setDepth(pixelIndex, depth);
		setPixel(pixelIndex*3, pixelColour);
	}

}


void FrameBuffer::setPixel(int index, glm::vec3 colour) {
	data[index + 0] = colour.x;
	data[index + 1] = colour.y;
	data[index + 2] = colour.z;	
}

int FrameBuffer::getIndex(int xCoord, int yCoord) {
	return xCoord + (yCoord * width);
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


glm::vec3 getPixelData(float xUV, float yUV, utils::Texture texture) {
	int texX = static_cast<int>(xUV * (constants::textureWidth - 1));
	int texY = static_cast<int>((1.0f - yUV) * (constants::textureHeight - 1));
	int pixelIndex = (texY * constants::textureWidth + texX) * texture.channels;
	int red = texture.data[pixelIndex];
	int green = texture.data[pixelIndex + 1];
	int blue = texture.data[pixelIndex + 2];

	if (texture.channels == 4) {
		if (texture.data[pixelIndex + 3] < 128) {
			return glm::vec3(-1.0f); //Pixel is transparent.
		}
	}
	
	return glm::vec3(red, green, blue);
}

}