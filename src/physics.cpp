#include "includes.h"
#include "utils.h"
using namespace std;
using namespace utils;

namespace physics {

glm::vec2 playerMove(glm::vec2 playerPosition, float playerViewAngle) {
	playerPosition.x += sin(playerViewAngle * constants::toRad); //Change in X
	playerPosition.y += cos(playerViewAngle * constants::toRad); //Change in Y
	return playerPosition;
};

bool checkCollision() {
	return false;
}

}