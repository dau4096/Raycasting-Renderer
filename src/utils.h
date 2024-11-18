#ifndef UTILS_H
#define UTILS_H

#include "includes.h"
#include <vector>
#include <stdexcept>
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

//Utility functions
namespace utils {
	void print(std::string str);
	void raise(std::string str);
	void pause();
	void GLErrorcheck(std::string location = "", bool shouldPause = false);


	float determinant(glm::vec2 vecA, glm::vec2 vecB);


	class FrameBuffer {
	public:
		FrameBuffer(int width, int height);

		unsigned char* operator[](int y);

		int getWidth();
		int getHeight();

		unsigned char* getData();

		void clearBuffer();
		void drawLine(int xCoord, int height, glm::vec3 colour);
		void setPixel(int index, glm::vec3 colour);
		int getIndex(int xCoord, int yCoord);

	private:
		int width, height;
		std::vector<unsigned char> data;
	};


	struct Wall {
		glm::vec2 start, end;
		glm::vec3 colour;

	    Wall() : start(0.0f, 0.0f), end(0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f) {}

		Wall(glm::vec2 start, glm::vec2 end, glm::vec3 colour)
			: start(start), end(end), colour(colour) {}
	};


	struct Ray {
		glm::vec2 position, direction, end;

		Ray(glm::vec2 position, glm::vec2 direction)
			: position(position), direction(direction) {}
	};


	void printFramebuffer(utils::FrameBuffer frameBuffer);

	bool saveTextureToFile(GLuint textureID, int width, int height, const std::string& filename);
}

#endif