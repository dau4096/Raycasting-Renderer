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


	struct Wall {
		glm::vec2 start, end;
		int textureID;

	    Wall() : start(0.0f, 0.0f), end(0.0f, 0.0f), textureID(0) {}

		Wall(glm::vec2 start, glm::vec2 end, int textureID)
			: start(start), end(end), textureID(textureID) {}
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

		unsigned char* getData();

		void clearBuffer();
		void drawLine(int xCoord, int height, utils::Wall wall, glm::vec2 position, unsigned char* textureData, int channels, float multiplier);
		void setPixel(int index, glm::vec3 colour);
		int getIndex(int xCoord, int yCoord);

	private:
		int width, height;
		std::vector<unsigned char> data;
	};


	void printFramebuffer(utils::FrameBuffer frameBuffer);

	bool saveTextureToFile(GLuint textureID, int width, int height, const std::string& filename);

	glm::vec3 getPixelData(float xUV, float yUV, unsigned char* textureData, int channels);
}

#endif