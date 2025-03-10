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
	void printVec3(glm::vec3 vector);
	void raise(std::string str);
	void pause();
	void GLErrorcheck(std::string location = "", bool shouldPause = false);


	float determinant(glm::vec2 vecA, glm::vec2 vecB);
	float angleClamp(float value); //Degrees


	int RNGc(); //Client
	int RNGw(); //World
	void clearRNG(); //Reset both
	


	struct Texture {
		glm::vec2 dimentions;
		int channels;
		unsigned char* data;
		int valid;

		Texture() : dimentions(0.0f, 0.0f), channels(0), data(nullptr), valid(0) {}

		Texture(glm::vec2 dimentions, int channels, unsigned char* data)
			: dimentions(dimentions), channels(channels), data(data), valid(1) {}
	};


	struct Visplane {
		glm::vec2 start;
		glm::vec2 end;
		float height;
		int textureID;
		int valid;
		float _padding;

		Visplane()
			: start(0.0f, 0.0f), end(0.0f, 0.0f), height(0.0f), textureID(0), valid(0), _padding{0.0f} {}

		Visplane(glm::vec2 start, glm::vec2 end, float heightZ, int textureID)
			: start(start), end(end), height(heightZ), textureID(textureID), valid(1), _padding{0.0f} {}
	};


	struct Wall {
		alignas(16) glm::vec3 start;
		alignas(16) glm::vec3 end;
		alignas(4) int textureID;
		alignas(4) int valid;
		alignas(8) float _padding[2];

		Wall()
			: start(0.0f, 0.0f, 0.0f), end(0.0f, 0.0f, 0.0f), textureID(0), valid(0), _padding{0.0f, 0.0f} {}

		Wall(glm::vec2 start, glm::vec2 end, float lowZ, float topZ, int textureID)
			: start(glm::vec3(start.x, start.y, lowZ)), end(glm::vec3(end.x, end.y, topZ)), textureID(textureID), valid(1), _padding{0.0f, 0.0f} {}
	};

	struct Sprite {
		alignas(16) glm::vec2 position;
		alignas(4) float width;
		alignas(4) int textureID;
		alignas(4) int valid;
		alignas(4) float _padding;

		Sprite() : position(0.0f, 0.0f), width(0.0f), textureID(0), valid(0), _padding(0.0f) {}

		Sprite(glm::vec2 position, float width, int textureID)
			: position(position), width(width), textureID(textureID), valid(1), _padding(0.0f) {}
	};

	struct Light {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 colour;
		alignas(4) float intensity;
		alignas(4) int valid;
		alignas(4) float _padding;

		Light() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), valid(0), _padding{0.0f} {}

		Light(glm::vec3 position, glm::vec3 colour, float intensity)
			: position(position), colour(colour), intensity(intensity), valid(1), _padding{0.0f} {}
	};


	struct Ray {
		glm::vec2 position, direction, end;

		Ray(glm::vec2 position, glm::vec2 direction)
			: position(position), direction(direction) {}
	};


	struct Player {
		glm::vec3 position;
		float viewAngle;

		Player(glm::vec3 position, float angle)
			: position(position), viewAngle(angle) {}
	};

}

#endif