#include "includes.h"
#include "utils.h"
using namespace std;
using namespace utils;


bool prevJump = false;
bool touchingFloorCheck = false;
const float EPSILON = 1e-5f;


namespace physics {

glm::vec2 raycast(utils::Ray ray, utils::Wall wall) {
	glm::vec2 wallStartV2 = glm::vec2(wall.start.x, wall.start.y);
	glm::vec2 wallEndV2 = glm::vec2(wall.end.x, wall.end.y);

	glm::vec2 xDiff = glm::vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	glm::vec2 yDiff = glm::vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);


	double divisor = utils::determinant(xDiff, yDiff);
	if (abs(divisor) < EPSILON) {
		//Lines do not intersect, as they are nearly parallel.
		return constants::INVALIDv2;
	}


	glm::vec2 dets = glm::vec2(utils::determinant(ray.position, ray.end), utils::determinant(wallStartV2, wallEndV2));
	double xCoord = utils::determinant(dets, xDiff) / divisor;
	double yCoord = utils::determinant(dets, yDiff) / divisor;

	glm::vec2 intersectPoint = glm::vec2(xCoord, yCoord);


	//Check if the intersection is within the wall.
	if ((intersectPoint.x < min(wall.start.x, wall.end.x)) || (intersectPoint.x > max(wall.start.x, wall.end.x)) ||
		(intersectPoint.y < min(wall.start.y, wall.end.y)) || (intersectPoint.y > max(wall.start.y, wall.end.y))) {
		return constants::INVALIDv2;
	}


	glm::vec2 intersectDirection = glm::normalize(intersectPoint - ray.position);
	glm::vec2 directionDifference = ray.direction - intersectDirection;

	
	if (glm::length(directionDifference) < EPSILON) {
		//Wrong way, behind camera.
		return constants::INVALIDv2;
	}
	

	return intersectPoint;  
}


bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius) {
	glm::vec2 lineStartV2 = glm::vec2(line.start.x, line.start.y);
	glm::vec2 lineEndV2 = glm::vec2(line.end.x, line.end.y);

	glm::vec2 lineDir = lineEndV2 - lineStartV2;
	glm::vec2 lineToCircle = circlePosition - lineStartV2;

	float t = glm::dot(lineToCircle, lineDir) / glm::dot(lineDir, lineDir);
	t = glm::clamp(t, 0.0f, 1.0f);

	glm::vec2 closestPoint = lineStartV2 + t * lineDir;
	float distToCircle = glm::length(circlePosition - closestPoint);

	return distToCircle <= radius;
}



float quadraticFormula(float a, float b, float determinant, bool positiveSolution=true) {
	float sign = (positiveSolution) ? 1.0f : -1.0f;
	//(-b +/- sqrt(b^2 - 4ac)) / (2a)
	return (-b + (sign * sqrt(determinant))) / (2*a);
}



void playerMove(
		utils::Player *player,
		unordered_map<int, bool> keyMap,
		std::array<utils::Wall, constants::MAX_WALLS>*wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData
	) {

	if (player->position.z <= constants::KILL_PLANE_HEIGHT) {
		//Reset player.
		player->position = playerConfig::PLAYER_START_POSITION;
		player->velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		player->viewAngle = playerConfig::PLAYER_START_ANGLE;
		player->state = E_NONE;
		player->touchingFloor = false;
		player->health = playerConfig::PLAYER_MAX_HEALTH;
		player->energy = playerConfig::PLAYER_MAX_ENERGY;
	}

	float newX = 0.0f;
	float newY = 0.0f;

	float playerSpeed = playerConfig::MOVE_SPEED_BASE;
	float maxV = playerConfig::MOVE_SPEED_BASE;

	if (keyMap[GLFW_KEY_LEFT_SHIFT]) {
		playerSpeed *= playerConfig::MOVE_SPEED_RUN_MULT;
		maxV = playerConfig::MOVE_SPEED_BASE * playerConfig::MOVE_SPEED_RUN_MULT;
	}

	playerSpeed = glm::clamp(maxV / playerSpeed, 0.0f, maxV);

	// Determine the movement vector based on key presses
	if (keyMap[GLFW_KEY_W]) {
		float reduction = (keyMap[GLFW_KEY_A] || keyMap[GLFW_KEY_D]) ? 0.70710678f : 1.0f;
		newX += playerSpeed * sin(player->viewAngle * constants::TO_RAD) * reduction;
		newY += playerSpeed * cos(player->viewAngle * constants::TO_RAD) * reduction;
	}
	if (keyMap[GLFW_KEY_S]) {
		float reduction = (keyMap[GLFW_KEY_A] || keyMap[GLFW_KEY_D]) ? 0.70710678f : 1.0f;
		newX -= playerSpeed * sin(player->viewAngle * constants::TO_RAD) * reduction;
		newY -= playerSpeed * cos(player->viewAngle * constants::TO_RAD) * reduction;
	}
	if (keyMap[GLFW_KEY_A]) {
		float reduction = (keyMap[GLFW_KEY_W] || keyMap[GLFW_KEY_S]) ? 0.70710678f : 1.0f;
		newX -= playerSpeed * cos((player->viewAngle) * constants::TO_RAD) * reduction;
		newY -= playerSpeed * -sin((player->viewAngle) * constants::TO_RAD) * reduction;
	}
	if (keyMap[GLFW_KEY_D]) {
		float reduction = (keyMap[GLFW_KEY_W] || keyMap[GLFW_KEY_S]) ? 0.70710678f : 1.0f;
		newX += playerSpeed * cos((player->viewAngle) * constants::TO_RAD) * reduction;
		newY += playerSpeed * -sin((player->viewAngle) * constants::TO_RAD) * reduction;
	}
	if (keyMap[GLFW_KEY_SPACE] && !prevJump && touchingFloorCheck) {
		player->velocity.z += playerConfig::JUMP_INIT_SPEED;
		player->position.z += 0.025;
	}


	glm::vec2 vAddition = glm::vec2(newX, newY);
	if (length(vAddition) > playerSpeed) {
		vAddition = normalize(vAddition) * playerSpeed;
	}
	

	//Occasionally returns NaN somehow.
	glm::vec2 curVelocity = glm::vec2(player->velocity.x, player->velocity.y);
	if (length(curVelocity) > playerConfig::MAX_AIR_SPEED_XY && length(vAddition) > EPSILON) {
		glm::vec2 vRight = glm::normalize(glm::vec2(player->velocity.y, -player->velocity.x));
		vAddition = vRight * glm::dot(glm::normalize(vAddition), vRight) * playerSpeed;
	}
	player->velocity.x += vAddition.x; player->velocity.y += vAddition.y;



	prevJump = keyMap[GLFW_KEY_SPACE];
	touchingFloorCheck = false;




	float playerFootZ = player->position.z - (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
	float playerHeadZ = player->position.z + (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);


	//Vertical Calculations;
	for (const utils::Visplane& plane : *visplaneData) {
		if (plane.valid < 1) {continue;}

		bool inPlaneXYRange = !(
			(player->position.x + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(plane.start.x, plane.end.x))
			|| (player->position.x - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(plane.start.x, plane.end.x))
			|| (player->position.y + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(plane.start.y, plane.end.y))
			|| (player->position.y - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(plane.start.y, plane.end.y))
		); // !outOfRange.
		bool abovePlane = player->position.z >= plane.height;

		if (inPlaneXYRange) {
			//If ΔZ < 0.42857u then allow player to climb up (stairs, ledge)
			float stepUpZ = plane.height - playerFootZ;
			if (stepUpZ <= constants::MAX_STEP_HEIGHT && stepUpZ >= 0.0f) {
				player->position.z += stepUpZ;
				player->velocity.z = 0.0f;
				touchingFloorCheck = true;
			} else if (abovePlane && playerFootZ < plane.height) {
				player->position.z = plane.height + (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
				player->velocity.z = 0.0f;
				touchingFloorCheck = true;
			} else if (!abovePlane && playerHeadZ > plane.height) {
				player->position.z = plane.height - (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
				player->velocity.z = 0.0f;
			}
		}
	}
	player->touchingFloor = touchingFloorCheck;



	//Horizontal Calculations;
	if (glm::length(player->velocity) < EPSILON) {
		return;
	}


	if (dev::NO_COLLIDE > 0) {
		player->position += glm::vec3(player->velocity.x, player->velocity.y, 0.0f);
		return;
	}


	glm::vec2 playerPosV2 = glm::vec2(player->position.x, player->position.y);
	for (const utils::Wall& wall : *wallData) {
		if ((wall.valid < 1) || (wall.specialType == W_TRIGGER)) {continue;}
		bool playerZCheckWall = !(
			(playerHeadZ < min(wall.start.z, wall.end.z))
			 || (playerFootZ + constants::MAX_STEP_HEIGHT > max(wall.start.z, wall.end.z))
		); //!aboveOrBelow.

		if (circleLineIntersect(wall, playerPosV2 + glm::vec2(player->velocity.x, player->velocity.y), playerConfig::PLAYER_COLLISION_RADIUS) && playerZCheckWall) {
			glm::vec2 wallDir = glm::normalize(wall.start - wall.end);
			glm::vec2 correctedV = wallDir * glm::dot(glm::normalize(glm::vec2(player->velocity.x, player->velocity.y)), wallDir) * playerSpeed;
			player->velocity.x = correctedV.x; player->velocity.y = correctedV.y;
		}
	}

	for (const utils::Sprite& sprite : *spriteData) {
		if ((sprite.valid < 1) || !(sprite.collision)) {continue;}
		const float spriteHeightTMP = 1.8f;
		float spriteHeadZ = sprite.position.z + (spriteHeightTMP/2.0f);
		float spriteFootZ = sprite.position.z - (spriteHeightTMP/2.0f);

		if (playerFootZ > spriteHeadZ || playerHeadZ < spriteFootZ) {continue; /* Above/Below sprite. */}


		glm::vec2 spritePosV2 = glm::vec2(sprite.position.x, sprite.position.y);
		glm::vec2 spriteDir = spritePosV2 - playerPosV2;
		float radius = playerConfig::PLAYER_COLLISION_RADIUS + sprite.width;

		if (glm::length(spriteDir) > radius) {continue;}

		//Uses b^2 - 4ac and compares to 0.
		float a = (player->velocity.x*player->velocity.x) + (player->velocity.y*player->velocity.y);
		float b = 2 * glm::dot(spriteDir, glm::vec2(player->velocity.x, player->velocity.y));
		float c = spriteDir.x*spriteDir.x + spriteDir.y*spriteDir.y - radius*radius;

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

		glm::vec2 intersectPoint = playerPosV2 + glm::vec2(player->velocity.x, player->velocity.y) * Mu;
		glm::vec2 normal = glm::normalize(intersectPoint - spritePosV2);
		glm::vec2 movementAlongNormal = glm::dot(glm::vec2(player->velocity.x, player->velocity.y), normal) * normal;
		glm::vec2 correctedV = glm::vec2(player->velocity.x, player->velocity.y) - movementAlongNormal*0.75f;
		player->velocity.x = correctedV.x; player->velocity.y = correctedV.y;
	}


	if (touchingFloorCheck) {player->velocity.x *= constants::FLOOR_FRICT_COEFF; player->velocity.y *= constants::FLOOR_FRICT_COEFF;}
	else {player->velocity *= constants::AIR_FRICT_COEFF;}
	player->velocity.z -= constants::GRAVITY_ACCEL / static_cast<float>(constants::DT);
	player->position += player->velocity;

	return;
};




void updateSpecials(
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData,
		utils::Player *player, std::unordered_map<int, bool> keyMap
	) {


	for (utils::Wall& wall : *wallData) {
		if ((wall.specialType == W_INVALID) || (wall.specialType == W_NORMAL)) {continue;}
		bool enabled = *(wall.IOPtr) == 1;

		switch(wall.specialType) {
			case W_TRIGGER:{
				float playerFootZ = player->position.z - (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
				float playerHeadZ = player->position.z + (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
				glm::vec2 playerPosV2 = glm::vec2(player->position.x, player->position.y);
				bool playerZCheckWall = !(
					(playerHeadZ < (std::min(wall.start.z, wall.end.z)))
					 || (playerFootZ + constants::MAX_STEP_HEIGHT > (std::max(wall.start.z, wall.end.z)))
				); //!aboveOrBelow.

				if (circleLineIntersect(wall, playerPosV2 + glm::vec2(player->velocity.x, player->velocity.y), playerConfig::PLAYER_COLLISION_RADIUS) && playerZCheckWall) {
					*(wall.IOPtr) = 1;
				} else {
					*(wall.IOPtr) = 0;
				}
				break;
			}

			case W_SWITCH: { //Check for interaction with wall.
				glm::vec2 dir = glm::vec2(sin(player->viewAngle), cos(player->viewAngle));
				utils::Ray ray = utils::Ray(player->position, dir, playerConfig::PLAYER_INTERACT_RAY_DIST);
				if (keyMap[GLFW_KEY_E] && (raycast(ray, wall) != constants::INVALIDv2)) {
					wall.internal = (wall.internal == 1) ? 0 : 1;
				}
				*(wall.IOPtr) = wall.internal;
				break;
			}

			case W_MOVEV_FAST: { //Move vertically, quickly.
				float speed = constants::SPECIAL_MOVE_SPEED_FAST;
				if (wall.data < 0) { //Downwards movement.
					if (enabled && (wall.internal > wall.data)) {
						wall.start.z -= speed;
						wall.end.z -= speed;
						wall.internal -= speed;
					} else if (!enabled && (wall.internal < 0)) {
						wall.start.z += speed;
						wall.end.z += speed;
						wall.internal += speed;
					}
				} else { //Upwards movement
					if (enabled && (wall.internal < wall.data)) {
						wall.start.z += speed;
						wall.end.z += speed;
						wall.internal += speed;
					} else if (!enabled && (wall.internal > 0)) {
						wall.start.z -= speed;
						wall.end.z -= speed;
						wall.internal -= speed;
					}
				}
				break;
			}

			case W_MOVEV_SLOW: { //Move vertically, slowly.
				float speed = constants::SPECIAL_MOVE_SPEED_SLOW;
				if (wall.data < 0) { //Downwards movement.
					if (enabled && (wall.internal > wall.data)) {
						wall.start.z -= speed;
						wall.end.z -= speed;
						wall.internal -= speed;
					} else if (!enabled && (wall.internal < 0)) {
						wall.start.z += speed;
						wall.end.z += speed;
						wall.internal += speed;
					}
				} else { //Upwards movement
					if (enabled && (wall.internal < wall.data)) {
						wall.start.z += speed;
						wall.end.z += speed;
						wall.internal += speed;
					} else if (!enabled && (wall.internal > 0)) {
						wall.start.z -= speed;
						wall.end.z -= speed;
						wall.internal -= speed;
					}
				}
				break;
			}

			case W_MOVEH_FAST: { //Move horizontally (+/- wall direction) quickly.
				glm::vec2 wallDirv2 = glm::normalize(wall.start - wall.end);
				glm::vec3 wallDir = glm::vec3(wallDirv2.x, wallDirv2.y, 0.0f);
				float speed = constants::SPECIAL_MOVE_SPEED_FAST;
				if (wall.data < 0) { //Movement towards start.
					if (enabled && (wall.internal > wall.data)) {
						wall.start -= wallDir * speed;
						wall.end -= wallDir * speed;
						wall.internal -= speed;
					} else if (!enabled && (wall.internal < 0)) {
						wall.start += wallDir * speed;
						wall.end += wallDir * speed;
						wall.internal += speed;
					}
				} else { //Movement towards end.
					if (enabled && (wall.internal < wall.data)) {
						wall.start += wallDir * speed;
						wall.end += wallDir * speed;
						wall.internal += speed;
					} else if (!enabled && (wall.internal > 0)) {
						wall.start -= wallDir * speed;
						wall.end -= wallDir * speed;
						wall.internal -= speed;
					}
				}
				break;
			}

			case W_MOVEH_SLOW: {//Move horizontally (+/- wall direction) slowly.
				glm::vec2 wallDirv2 = glm::normalize(wall.start - wall.end);
				glm::vec3 wallDir = glm::vec3(wallDirv2.x, wallDirv2.y, 0.0f);
				float speed = constants::SPECIAL_MOVE_SPEED_SLOW;
				if (wall.data < 0) { //Movement towards start.
					if (enabled && (wall.internal > wall.data)) {
						wall.start -= wallDir * speed;
						wall.end -= wallDir * speed;
						wall.internal -= speed;
					} else if (!enabled && (wall.internal < 0)) {
						wall.start += wallDir * speed;
						wall.end += wallDir * speed;
						wall.internal += speed;
					}
				} else { //Movement towards end.
					if (enabled && (wall.internal < wall.data)) {
						wall.start += wallDir * speed;
						wall.end += wallDir * speed;
						wall.internal += speed;
					} else if (!enabled && (wall.internal > 0)) {
						wall.start -= wallDir * speed;
						wall.end -= wallDir * speed;
						wall.internal -= speed;
					}
				}
				break;
			}

			default:
				break;
		}
	}


	for (utils::Visplane& plane : *visplaneData) {
		if ((plane.specialType == V_INVALID) || (plane.specialType == V_NORMAL)) {continue;}
		bool enabled = *(plane.IOPtr) == 1;


		bool inPlaneXYRange = !(
			(player->position.x + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(plane.start.x, plane.end.x))
			|| (player->position.x - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(plane.start.x, plane.end.x))
			|| (player->position.y + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(plane.start.y, plane.end.y))
			|| (player->position.y - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(plane.start.y, plane.end.y))
		); // !outOfRange.
		bool abovePlane = player->position.z >= plane.height;
		bool planeTouch = false;

		if (inPlaneXYRange) {
			float playerFootZ = player->position.z - (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
			float playerHeadZ = player->position.z + (playerConfig::PLAYER_COLLISION_HEIGHT/2.0f);
			glm::vec2 playerPosV2 = glm::vec2(player->position.x, player->position.y);
			//If ΔZ < 0.42857u then allow player to climb up (stairs, ledge)
			float stepUpZ = plane.height - playerFootZ;
			if (stepUpZ <= constants::MAX_STEP_HEIGHT && stepUpZ >= 0.0f) {
				planeTouch = true;
			} else if (abovePlane && playerFootZ < plane.height) {
				planeTouch = true;
			} else if (!abovePlane && playerHeadZ > plane.height) {
				planeTouch = true;
			}
		}


		switch(plane.specialType) {
			case V_TRIGGER: {
				if (planeTouch) {
					*(plane.IOPtr) = 1;
				} else {
					*(plane.IOPtr) = 0;
				}
				break;
			}

			case V_MOVEV_FAST: { //Move vertically, quickly.
				float speed = constants::SPECIAL_MOVE_SPEED_FAST;
				if (plane.data < 0) { //Downwards movement.
					if (enabled && (plane.internal > plane.data)) {
						plane.height -= speed;
						plane.internal -= speed;
					} else if (!enabled && (plane.internal < 0)) {
						plane.height += speed;
						plane.internal += speed;
					}
				} else { //Upwards movement
					if (enabled && (plane.internal < plane.data)) {
						plane.height += speed;
						plane.internal += speed;
					} else if (!enabled && (plane.internal > 0)) {
						plane.height -= speed;
						plane.internal -= speed;
					}
				}
				break;
			}

			case V_MOVEV_SLOW: { //Move vertically, slowly.
				float speed = constants::SPECIAL_MOVE_SPEED_SLOW;
				if (plane.data < 0) { //Downwards movement.
					if (enabled && (plane.internal > plane.data)) {
						plane.height -= speed;
						plane.internal -= speed;
					} else if (!enabled && (plane.internal < 0)) {
						plane.height += speed;
						plane.internal += speed;
					}
				} else { //Upwards movement
					if (enabled && (plane.internal < plane.data)) {
						plane.height += speed;
						plane.internal += speed;
					} else if (!enabled && (plane.internal > 0)) {
						plane.height -= speed;
						plane.internal -= speed;
					}
				}
				break;
			}

			case V_HURT: {
				if (planeTouch) {
					float hurt = plane.data;
					utils::hurtPlayer(player, hurt);
				}
				break;
			}

			default:
				break;
		}
	}
}

}