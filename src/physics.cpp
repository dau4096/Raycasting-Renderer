#include "includes.h"
#include "utils.h"
#include "raycasting.h"
using namespace std;
using namespace utils;

namespace physics {

bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius) {
	glm::vec2 lineDir = line.end - line.start;  // Direction vector of the line segment
	glm::vec2 lineToCircle = circlePosition - line.start; // Vector from line start to circle center

	// Project lineToCircle onto lineDir to find the closest point on the line
	float t = glm::dot(lineToCircle, lineDir) / glm::dot(lineDir, lineDir);

	// Clamp t to [0, 1] to restrict to the line segment
	t = glm::clamp(t, 0.0f, 1.0f);

	// Find the closest point on the line segment
	glm::vec2 closestPoint = line.start + t * lineDir;

	// Calculate the distance from the circle's center to the closest point
	float distToCircle = glm::length(circlePosition - closestPoint);

	// Check if the distance is less than or equal to the radius
	return distToCircle <= radius;
}


utils::Player playerMove(utils::Player player, unordered_map<int, bool> keyMap, const std::array<utils::Wall, 128>* wallData) {
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

	glm::vec2 movementVector(newX, newY);
	if (glm::length(movementVector) < 1e-5) {
		return player;
	}


	if (dev::noCollis == 1.0f) {
		player.position += movementVector;
		return player;
	}


	movementVector = glm::normalize(movementVector) * playerSpeed;



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