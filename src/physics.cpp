#include "includes.h"
#include "utils.h"
using namespace std;
using namespace utils;

namespace physics {

glm::vec2 playerMove(glm::vec2 playerPosition, float playerViewAngle, unordered_map<int, bool> keyMap) {
	if (keyMap[GLFW_KEY_W]) {
		playerPosition.x += 0.05f /*player::moveSpeed*/ * sin(playerViewAngle * constants::toRad); //Change in X
		playerPosition.y += 0.05f /*player::moveSpeed*/ * cos(playerViewAngle * constants::toRad); //Change in Y
	}
	if (keyMap[GLFW_KEY_S]) {
		playerPosition.x -= 0.05f /*player::moveSpeed*/ * sin(playerViewAngle * constants::toRad); //Change in X
		playerPosition.y -= 0.05f /*player::moveSpeed*/ * cos(playerViewAngle * constants::toRad); //Change in Y
	}
	if (keyMap[GLFW_KEY_A]) {
		playerPosition.x -= 0.05f /*player::moveSpeed*/ * sin((playerViewAngle + 90.0f) * constants::toRad); //Change in X
		playerPosition.y -= 0.05f /*player::moveSpeed*/ * cos((playerViewAngle + 90.0f) * constants::toRad); //Change in Y
	}
	if (keyMap[GLFW_KEY_D]) {
		playerPosition.x += 0.05f /*player::moveSpeed*/ * sin((playerViewAngle + 90.0f) * constants::toRad); //Change in X
		playerPosition.y += 0.05f /*player::moveSpeed*/ * cos((playerViewAngle + 90.0f) * constants::toRad); //Change in Y
	}
	return playerPosition;
};


bool checkCollision() {
	return false;
}


float angleClamp(float value) {
	if (value < 0.0) {
		return 360 + value;
	}
	return fmod(value, 360.0);
}

}