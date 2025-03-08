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



float quadraticFormula(float a, float b, float determinant, bool positiveSolution=true) {
	float sign = (positiveSolution) ? 1.0f : -1.0f;
	// (-b +/- sqrt(b^2 - 4ac)) / 2a
	return (-b + (sign * sqrt(determinant))) / 2*a;
}



utils::Player playerMove(utils::Player player, unordered_map<int, bool> keyMap, const std::array<utils::Wall, 256>* wallData, std::vector<utils::Sprite>* spriteData) {
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
		newX -= playerSpeed * cos((player.viewAngle) * constants::toRad);
		newY -= playerSpeed * -sin((player.viewAngle) * constants::toRad);
	}
	if (keyMap[GLFW_KEY_D]) {
		newX += playerSpeed * cos((player.viewAngle) * constants::toRad);
		newY += playerSpeed * -sin((player.viewAngle) * constants::toRad);
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
	for (const utils::Wall& wall : *wallData) {
		if (circleLineIntersect(wall, player.position + movementVector, playerConfig::minCollisionDist)) {
			glm::vec2 wallDir = glm::normalize(wall.start - wall.end);
			movementVector = wallDir * glm::dot(glm::normalize(movementVector), wallDir) * playerSpeed;
		}
	}

	for (const utils::Sprite& sprite : *spriteData) {
		glm::vec2 dir = sprite.position - player.position;
		float radius = playerConfig::minCollisionDist + sprite.width;

		if (glm::length(dir) > radius) {continue;}

		//Uses b^2 - 4ac and compares to 0.
		float a = (movementVector.x*movementVector.x) + (movementVector.y*movementVector.y);
		float b = 2 * glm::dot(dir, movementVector);
		float c = dir.x*dir.x + dir.y*dir.y - radius*radius;

		float determinant = b*b - 4*a*c;
		float Mu;

		if (determinant > 0.0f) { //2 intersect points
			float positive = quadraticFormula(a, b, determinant);
			float negative = quadraticFormula(a, b, determinant, false);
			Mu = min(positive, negative);

		} else if (determinant == 0) { //1 intersect point
			Mu = quadraticFormula(a, b, determinant); //Take the positive root.

		} else { //No intersects; no collision.
			Mu = 0.0f;
		}

		glm::vec2 intersectPoint = player.position + movementVector * Mu;
		glm::vec2 normal = glm::normalize(intersectPoint - sprite.position);
		glm::vec2 movementAlongNormal = glm::dot(movementVector, normal) * normal;
		movementVector -= movementAlongNormal;

		if (glm::length(movementVector) > playerSpeed) {
			movementVector = glm::normalize(movementVector) * playerSpeed;
		}
	}


	player.position += movementVector;

	return player;
};


}