#include "includes.h"
#include "utils.h"
using namespace std;
using namespace utils;

namespace physics {

bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius) {
	glm::vec2 lineDir = line.end - line.start;
	glm::vec2 lineToCircle = circlePosition - line.start;

	float t = glm::dot(lineToCircle, lineDir) / glm::dot(lineDir, lineDir);
	t = glm::clamp(t, 0.0f, 1.0f);

	glm::vec2 closestPoint = line.start + t * lineDir;
	float distToCircle = glm::length(circlePosition - closestPoint);

	return distToCircle <= radius;
}


utils::Player playerMove(utils::Player player, unordered_map<int, bool> keyMap, const std::array<utils::Wall, 256>* wallData) {
	float newX = 0.0f;
	float newY = 0.0f;

	float playerSpeed = playerConfig::moveSpeed;

	if (keyMap[GLFW_KEY_LEFT_SHIFT]) {
		playerSpeed *= playerConfig::runMultiplier;
	}

	// Determine the movement vector based on key presses
	if (keyMap[GLFW_KEY_W]) {
		newX += playerSpeed * sin(player.viewAngle * constants::toRad);
		newY += playerSpeed * cos(player.viewAngle * constants::toRad);
	}
	if (keyMap[GLFW_KEY_S]) {
		newX -= playerSpeed * sin(player.viewAngle * constants::toRad);
		newY -= playerSpeed * cos(player.viewAngle * constants::toRad);
	}
	if (keyMap[GLFW_KEY_A]) {
		newX -= playerSpeed * sin((player.viewAngle + 90.0f) * constants::toRad);
		newY -= playerSpeed * cos((player.viewAngle + 90.0f) * constants::toRad);
	}
	if (keyMap[GLFW_KEY_D]) {
		newX += playerSpeed * sin((player.viewAngle + 90.0f) * constants::toRad);
		newY += playerSpeed * cos((player.viewAngle + 90.0f) * constants::toRad);
	}

	glm::vec2 movementVector = glm::vec2(newX, newY);
	if (glm::length(movementVector) < 1e-5) {
		return player;
	}
	movementVector = glm::normalize(movementVector) * playerSpeed;


	if (dev::noCollis == 1.0f) {
		player.position += movementVector;
		return player;
	}





	float maxAllowedDistance = glm::length(movementVector);
	bool collided = false;
	for (const utils::Wall& wall : *wallData) {
		if (circleLineIntersect(wall, player.position + movementVector, playerConfig::minCollisionDist)) {
			collided = true;
		}
	}


	if (!collided) {
		player.position += movementVector;
	}

	return player;
};


}