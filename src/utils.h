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


	class FrameBuffer {
	public:
		FrameBuffer(int width, int height);

		unsigned char* operator[](int y);

		int getWidth();
		int getHeight();

		unsigned char* getData();

		void clearBuffer();

	private:
		int width, height;
		std::vector<unsigned char> data; // 2D vector for storing color values
	};

	struct Line {
		glm::vec2 start, end;
		unsigned char colour;
	};

	struct Ray {
		glm::vec2 position, direction;
	};

	void printFramebuffer(utils::FrameBuffer frameBuffer);

	bool saveTextureToFile(GLuint textureID, int width, int height, const std::string& filename);
}

#endif