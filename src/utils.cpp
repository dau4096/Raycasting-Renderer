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

}