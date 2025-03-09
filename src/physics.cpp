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
	return (-b + (sign * sqrt(determinant))) / (2*a);
}



utils::Player playerMove(utils::Player player, unordered_map<int, bool> keyMap, const std::array<utils::Wall, 256>* wallData, std::vector<utils::Sprite>* spriteData) {
	float newX = 0.0f;
	float newY = 0.0f;
	float newZ = 0.0f;

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
		player.position += glm::vec3(movementVector.x, movementVector.y, newZ);
		return player;
	}




	//Horizontal Calculations;
	glm::vec2 playerPosV2 = glm::vec2(player.position.x, player.position.y);
	for (const utils::Wall& wall : *wallData) {
		if (circleLineIntersect(wall, playerPosV2 + movementVector, playerConfig::minCollisionDist)) {
			glm::vec2 wallDir = glm::normalize(wall.start - wall.end);
			movementVector = wallDir * glm::dot(glm::normalize(movementVector), wallDir) * playerSpeed;
		}
	}

	for (const utils::Sprite& sprite : *spriteData) {
		glm::vec2 dir = sprite.position - playerPosV2;
		float radius = playerConfig::minCollisionDist + sprite.width;

		if (glm::length(dir) > radius) {continue;}

		//Uses b^2 - 4ac and compares to 0.
		float a = (movementVector.x*movementVector.x) + (movementVector.y*movementVector.y);
		float b = 2 * glm::dot(dir, movementVector);
		float c = dir.x*dir.x + dir.y*dir.y - radius*radius;

		float determinant = b*b - 4*a*c;
		float Mu = std::numeric_limits<float>::max();

		if (determinant > 0.0f) { //2 intersect points
			float root1 = quadraticFormula(a, b, determinant);
			float root2 = quadraticFormula(a, b, determinant, false);

			if (root1 > 0.0f && root1 < 1.0f) Mu = root1;
			if (root2 > 0.0f && root2 < 1.0f) Mu = std::min(Mu, root2);

			if (Mu == std::numeric_limits<float>::max()) Mu = 0.0f;

		} else { //1 or 0 intersects; no collision.
			Mu = 0.0f;
		}

		glm::vec2 intersectPoint = playerPosV2 + movementVector * Mu;
		glm::vec2 normal = glm::normalize(intersectPoint - sprite.position);
		glm::vec2 movementAlongNormal = glm::dot(movementVector, normal) * normal;
		movementVector -= movementAlongNormal*0.75f;

		if (glm::length(movementVector) > playerSpeed) {
			movementVector = glm::normalize(movementVector) * playerSpeed;
		}
	}


	//Vertical Calculations; (N/A for now.)
	float vMove = newZ;


	player.position += glm::vec3(movementVector.x, movementVector.y, vMove);

	return player;
};


}