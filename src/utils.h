#ifndef UTILS_H
#define UTILS_H

#include "includes.h"
#include <vector>
#include <stdexcept>
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

//Utility functions
namespace utils {
	void print(std::string str);
	void printVec2(glm::vec2 vector);
	void raise(std::string str);
	void pause();
	void GLErrorcheck(std::string location = "", bool shouldPause = false);


	float determinant(glm::vec2 vecA, glm::vec2 vecB);
	float angleClamp(float value);


	struct Texture {
		glm::vec2 dimentions;
		int channels;
		unsigned char* data;
		bool valid;

		Texture() : dimentions(0.0f, 0.0f), channels(0), data(nullptr), valid(false) {}

		Texture(glm::vec2 dimentions, int channels, unsigned char* data)
			: dimentions(dimentions), channels(channels), data(data), valid(true) {}
	};


	struct Wall {
		glm::vec2 start, end;
		int textureID;
		bool valid;

	    Wall() : start(0.0f, 0.0f), end(0.0f, 0.0f), textureID(0), valid(false) {}

		Wall(glm::vec2 start, glm::vec2 end, int textureID)
			: start(start), end(end), textureID(textureID), valid(true) {}
	};

	struct Sprite {
		glm::vec2 position;
		float width;
		int textureID;
		bool valid;

		Sprite() : position(0.0f, 0.0f), width(0.0f), textureID(0), valid(false) {}

		Sprite(glm::vec2 position, float width, int textureID)
			: position(position), width(width), textureID(textureID), valid(true) {}
	};


	struct Ray {
		glm::vec2 position, direction, end;

		Ray(glm::vec2 position, glm::vec2 direction)
			: position(position), direction(direction) {}
	};


	struct Player {
		glm::vec2 position;
		float viewAngle;

		Player(glm::vec2 position, float angle)
			: position(position), viewAngle(angle) {}
	};


	class FrameBuffer {
	public:
		FrameBuffer(int width, int height);

		unsigned char* operator[](int y);

		int getWidth();
		int getHeight();

		void setDepth(int index, float depth);
		float getDepth(int index);

		unsigned char* getData();

		void clearBuffer();
		void drawWallLine(int xCoord, int lineHeight, utils::Wall wall, glm::vec2 position, float depth, utils::Texture texture, float multiplier);
		void drawSpriteLine(int xCoord, int lineHeight, utils::Sprite sprite, int spriteX, float spriteWidth, float depth, utils::Texture texture);
		void setPixel(int index, glm::vec3 colour);
		int getIndex(int xCoord, int yCoord);

	private:
		int width, height;
		std::vector<unsigned char> data;
		std::vector<float> depths;
	};


	void printFramebuffer(utils::FrameBuffer frameBuffer);

	glm::vec3 getPixelData(float xUV, float yUV, utils::Texture texture);
}

#endif