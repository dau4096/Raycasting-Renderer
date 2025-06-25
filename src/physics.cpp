#include "includes.h"
#include "global.h"
#include "utils.h"
using namespace std;
using namespace utils;


bool prevJump = false, prevSlide = false;
bool touchingFloorCheck = false;
const float EPSILON = 1e-5f;
float desiredLeanLR = 0.0f, leanLRCurrent = 0.0f;
float desiredLeanFB = 0.0f, leanFBCurrent = 0.0f;




float cross2D(glm::vec2 a, glm::vec2 b) {
	return a.x * b.y - a.y * b.x;
}



bool quickIntersect(glm::vec3 pointA, glm::vec3 pointB, utils::Wall wall, float* distSQ) {
	glm::vec2 rayDelta = glm::vec2(pointB) - glm::vec2(pointA);
	glm::vec2 wallDelta = glm::vec2(wall.end) - glm::vec2(wall.start);

	float denom = cross2D(rayDelta, wallDelta);
	if (abs(denom) < 1e-4f) {return false;}

	glm::vec2 rel = glm::vec2(wall.start) - glm::vec2(pointA);
	float t = cross2D(rel, wallDelta) / denom;
	float u = cross2D(rel, rayDelta) / denom;

	if ((t < -1e-4f) || (t > 1.0f + 1e-4f) || (u < -1e-4f) || (u > 1.0f + 1e-4f)) {return false;}

	float z = pointA.z + (pointB.z - pointA.z) * t;

	if ((min(wall.start.z, wall.end.z) - 1e-4f <= z) && (z <= max(wall.start.z, wall.end.z) + 1e-4f)) {
		if (distSQ != nullptr) {
			glm::vec2 intersectPoint = glm::vec2(wall.start) + wallDelta * t;
			glm::vec2 distDelta = intersectPoint - glm::vec2(pointA);
			*distSQ = glm::dot(distDelta, distDelta);
		}
		return true;
	}
	return false;
}



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




bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius, float* distToLine=nullptr) {
	glm::vec2 lineStartV2 = glm::vec2(line.start.x, line.start.y);
	glm::vec2 lineEndV2 = glm::vec2(line.end.x, line.end.y);

	glm::vec2 lineDir = lineEndV2 - lineStartV2;
	glm::vec2 lineToCircle = circlePosition - lineStartV2;

	float t = glm::dot(lineToCircle, lineDir) / glm::dot(lineDir, lineDir);
	t = glm::clamp(t, 0.0f, 1.0f);

	glm::vec2 closestPoint = lineStartV2 + t * lineDir;
	float distToCircle = glm::length(circlePosition - closestPoint);

	if (distToLine) {
		*distToLine = distToCircle;
	}
	return distToCircle <= radius;
}



float quadraticFormula(float a, float b, float determinant, bool positiveSolution=true) {
	float sign = (positiveSolution) ? 1.0f : -1.0f;
	//(-b +/- sqrt(b^2 - 4ac)) / (2a)
	return (-b + (sign * sqrt(determinant))) / (2*a);
}





bool didHitSwitch(utils::Player* player, size_t switchIdx, std::vector<utils::Wall>* wallData) {
	glm::vec2 dir = glm::vec2(sin(player->viewAngle * constants::TO_RAD), cos(player->viewAngle * constants::TO_RAD));
	glm::vec2 interactDelta = dir * playerConfig::PLAYER_INTERACT_RAY_DIST;
	glm::vec2 playerV2 = glm::vec2(player->position);
	glm::vec2 end = playerV2 + interactDelta;
	float camPosZ = player->position.z + (player->height / 3.0f);

	bool closestWasSwitch = false;
	float minDistanceSQ = playerConfig::PLAYER_INTERACT_RAY_DIST*playerConfig::PLAYER_INTERACT_RAY_DIST;

	//Iterate through all the walls. (2D)
	for (size_t idx=0; idx<validWalls; idx++) {
		utils::Wall thisWall = wallData->at(idx);
		glm::vec2 wallStartV2 = glm::vec2(thisWall.start);
		glm::vec2 wallEndV2 = glm::vec2(thisWall.end);
		glm::vec2 direction = glm::normalize(wallEndV2 - wallStartV2);
		glm::vec2 wallNormal = glm::vec2(-direction.y, direction.x);

		//Check if ray even crosses wall in 2D.
		float projStart = glm::dot(playerV2 - wallStartV2, wallNormal);
		float projEnd = glm::dot(end - wallStartV2, wallNormal);
		if (projStart * projEnd >= 0.0f) {continue; /* Ray never crosses wall. */}

		//Calculate t-value
		float t = (projStart) / (projEnd - projStart);
		glm::vec2 intersectPoint = playerV2 - interactDelta * t;
		glm::vec3 minWall = glm::min(thisWall.start, thisWall.end);
		glm::vec3 maxWall = glm::max(thisWall.start, thisWall.end);
		if (
		    intersectPoint.x + EPSILON < minWall.x || intersectPoint.x - EPSILON > maxWall.x ||
		    intersectPoint.y + EPSILON < minWall.y || intersectPoint.y - EPSILON > maxWall.y ||
		    camPosZ + EPSILON < minWall.z || camPosZ - EPSILON > maxWall.z
		) {
			//Outside of valid wall segment.
			continue;
		}

		glm::vec2 d = intersectPoint - playerV2;
		float distSQ = glm::dot(d, d);
		if (distSQ >= minDistanceSQ) {continue;}

		minDistanceSQ = distSQ;
		closestWasSwitch = idx == switchIdx;
	}

	return closestWasSwitch;
}



namespace specialMotion {


//Walls
void applyWallDirectionalMovement(utils::Wall* wall, float speed, bool enabled) {
	glm::vec2 wallDir2D = glm::normalize(glm::vec2(wall->start) - glm::vec2(wall->end));
	glm::vec3 wallDir = glm::vec3(wallDir2D.x, wallDir2D.y, 0.0f);

	if (abs(wall->data/2.0f) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;

	} else if (wall->data/2.0f < 0) { //Movement toward wall.start.
		if (enabled && (wall->internal > wall->data/2.0f)) { //Turned on; move toward start.
			float newInternal = std::max(wall->internal - speed, wall->data/2.0f);
			float delta = wall->internal - newInternal;
			wall->start -= wallDir * delta;
			wall->end -= wallDir * delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(wall->internal + speed, 0.0f);
			float delta = newInternal - wall->internal;
			wall->start += wallDir * delta;
			wall->end += wallDir * delta;
			wall->internal = newInternal;
		}

	} else { //Movement toward wall.end.
		if (enabled && (wall->internal < wall->data/2.0f)) { //Turned on; move toward end.
			float newInternal = std::min(wall->internal + speed, wall->data/2.0f);
			float delta = newInternal - wall->internal;
			wall->start += wallDir * delta;
			wall->end += wallDir * delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::max(wall->internal - speed, 0.0f);
			float delta = wall->internal - newInternal;
			wall->start -= wallDir * delta;
			wall->end -= wallDir * delta;
			wall->internal = newInternal;
		}
	}
}


void applyWallNormalMovement(utils::Wall* wall, float speed, bool enabled) {
	glm::vec2 wallDir2D = glm::normalize(glm::vec2(wall->start) - glm::vec2(wall->end));
	glm::vec3 wallNormal = glm::vec3(-wallDir2D.y, wallDir2D.x, 0.0f);

	if (abs(wall->data/2.0f) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;

	} else if (wall->data/2.0f < 0) { //Movement toward wall.start.
		if (enabled && (wall->internal > wall->data/2.0f)) { //Turned on; move toward start.
			float newInternal = std::max(wall->internal - speed, wall->data/2.0f);
			float delta = wall->internal - newInternal;
			wall->start -= wallNormal * delta;
			wall->end -= wallNormal * delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(wall->internal + speed, 0.0f);
			float delta = newInternal - wall->internal;
			wall->start += wallNormal * delta;
			wall->end += wallNormal * delta;
			wall->internal = newInternal;
		}

	} else { //Movement toward wall.end.
		if (enabled && (wall->internal < wall->data/2.0f)) { //Turned on; move toward end.
			float newInternal = std::min(wall->internal + speed, wall->data/2.0f);
			float delta = newInternal - wall->internal;
			wall->start += wallNormal * delta;
			wall->end += wallNormal * delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::max(wall->internal - speed, 0.0f);
			float delta = wall->internal - newInternal;
			wall->start -= wallNormal * delta;
			wall->end -= wallNormal * delta;
			wall->internal = newInternal;
		}
	}
}


void applyWallZMovement(utils::Wall* wall, float speed, bool enabled) {
	if (abs(wall->data) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;

	} else if (wall->data < 0) { //Downwards
		if (enabled && (wall->internal > wall->data)) { //Turned on; moving down.
			float newInternal = std::max(wall->internal - speed, wall->data);
			float delta = wall->internal - newInternal;
			wall->start.z -= delta;
			wall->end.z -= delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(wall->internal + speed, 0.0f);
			float delta = newInternal - wall->internal;
			wall->start.z += delta;
			wall->end.z += delta;
			wall->internal = newInternal;
		}

	} else { //Upwards
		if (enabled && (wall->internal < wall->data)) { //Turned on; moving up.
			float newInternal = std::min(wall->internal + speed, wall->data);
			float delta = newInternal - wall->internal;
			wall->start.z += delta;
			wall->end.z += delta;
			wall->internal = newInternal;

		} else if (!enabled && (wall->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::max(wall->internal - speed, 0.0f);
			float delta = wall->internal - newInternal;
			wall->start.z -= delta;
			wall->end.z -= delta;
			wall->internal = newInternal;
		}
	}
}



//Visplanes
void applyVisplaneXMovement(utils::Visplane* vPlane, float speed, bool enabled) {
	if (abs(vPlane->data) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;


	} else if (vPlane->data < 0) { //Moving downwards.
		if (enabled && (vPlane->internal > vPlane->data)) { //Turned on; moving down.
			float newInternal = std::max(vPlane->internal - speed, vPlane->data);
			float delta = vPlane->internal - newInternal;
			vPlane->start.x -= delta;
			vPlane->end.x -= delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(vPlane->internal + speed, 0.0f);
			float delta = newInternal - vPlane->internal;
			vPlane->start.x += delta;
			vPlane->end.x += delta;
			vPlane->internal = newInternal;
		}

	} else { //Moving upwards.
		if (enabled && (vPlane->internal < vPlane->data)) { //Turned on; moving up.
			float newInternal = std::max(vPlane->internal + speed, 0.0f);
			float delta = newInternal - vPlane->internal;
			vPlane->start.x += delta;
			vPlane->end.x += delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::min(vPlane->internal - speed, vPlane->data);
			float delta = vPlane->internal - newInternal;
			vPlane->start.x -= delta;
			vPlane->end.x -= delta;
			vPlane->internal = newInternal;
		}
	}
}


void applyVisplaneYMovement(utils::Visplane* vPlane, float speed, bool enabled) {
	if (abs(vPlane->data) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;


	} else if (vPlane->data < 0) { //Moving downwards.
		if (enabled && (vPlane->internal > vPlane->data)) { //Turned on; moving down.
			float newInternal = std::max(vPlane->internal - speed, vPlane->data);
			float delta = vPlane->internal - newInternal;
			vPlane->start.y -= delta;
			vPlane->end.y -= delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(vPlane->internal + speed, 0.0f);
			float delta = newInternal - vPlane->internal;
			vPlane->start.y += delta;
			vPlane->end.y += delta;
			vPlane->internal = newInternal;
		}

	} else { //Moving upwards.
		if (enabled && (vPlane->internal < vPlane->data)) { //Turned on; moving up.
			float newInternal = std::max(vPlane->internal + speed, 0.0f);
			float delta = newInternal - vPlane->internal;
			vPlane->start.y += delta;
			vPlane->end.y += delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::min(vPlane->internal - speed, vPlane->data);
			float delta = vPlane->internal - newInternal;
			vPlane->start.y -= delta;
			vPlane->end.y -= delta;
			vPlane->internal = newInternal;
		}
	}
}


void applyVisplaneZMovement(utils::Visplane* vPlane, float speed, bool enabled) {
	if (abs(vPlane->data) < constants::SPECIAL_MOVE_SPEED_SLOW) { //Movement is not significant enough to carry out.
		return;


	} else if (vPlane->data < 0) { //Moving downwards.
		if (enabled && (vPlane->internal > vPlane->data)) { //Turned on; moving down.
			float newInternal = std::max(vPlane->internal - speed, vPlane->data);
			float delta = vPlane->internal - newInternal;
			vPlane->height -= delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal < 0)) { //Turned off; return to 0.
			float newInternal = std::min(vPlane->internal + speed, 0.0f);
			float delta = newInternal - vPlane->internal;
			vPlane->height += delta;
			vPlane->internal = newInternal;
		}

	} else { //Moving upwards.
		if (enabled && (vPlane->internal < vPlane->data)) { //Turned on; moving up.
			float newInternal = std::min(vPlane->internal + speed, vPlane->data);
			float delta = newInternal - vPlane->internal;
			vPlane->height += delta;
			vPlane->internal = newInternal;

		} else if (!enabled && (vPlane->internal > 0)) { //Turned off; return to 0.
			float newInternal = std::max(vPlane->internal - speed, 0.0f);
			float delta = vPlane->internal - newInternal;
			vPlane->height -= delta;
			vPlane->internal = newInternal;
		}
	}
}


}






namespace physics {

void playerMove(
		utils::Player *player,
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Sprite>* spriteData,
		std::vector<utils::Visplane>* visplaneData
	) {

	Player playerCopy = *player;

	if (player->position.z <= stageData.killPlaneZ) {
		//Reset player.
		player->position = stageData.playerStartPoint;
		player->velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		player->viewAngle = stageData.playerStartAngle;
		player->state = E_NONE;
		player->touchingFloor = false;
		player->health = playerConfig::PLAYER_MAX_HEALTH;
		player->energy = playerConfig::PLAYER_MAX_ENERGY;
	}

	float newX = 0.0f;
	float newY = 0.0f;
	glm::vec2 lateralMovement = glm::vec2(0.0f, 0.0f);

	float playerSpeed = playerConfig::MOVE_SPEED_BASE;
	float maxV = playerConfig::MOVE_SPEED_BASE;

	glm::vec2 XYDelta = glm::vec2(player->velocity.x, player->velocity.y);
	player->sliding = keyMap["MOVE_CROUCH"] && (glm::length(XYDelta) > playerConfig::SLIDE_THRESHOLD);
	if (keyMap["MOVE_CROUCH"]) {
		if (player->sliding) {
			if (!prevSlide && player->touchingFloor) {
				maxV *= playerConfig::MOVE_SPEED_SLIDE_ADD;
				glm::vec2 slideAddition = glm::normalize(XYDelta) * playerConfig::MOVE_SPEED_SLIDE_ADD;
				player->velocity += glm::vec3(slideAddition.x, slideAddition.y, 0.0f);
			}
		} else {
			playerSpeed *= playerConfig::MOVE_SPEED_CROUCH_MULT;
			maxV = playerConfig::MOVE_SPEED_BASE * playerConfig::MOVE_SPEED_CROUCH_MULT;
		}
	} else if (keyMap["MOVE_SPRINT"]) {
		playerSpeed *= playerConfig::MOVE_SPEED_RUN_MULT;
		maxV = playerConfig::MOVE_SPEED_BASE * playerConfig::MOVE_SPEED_RUN_MULT;
	}

	playerSpeed = glm::clamp(maxV / playerSpeed, 0.0f, maxV);

	// Determine the movement vector based on key presses
	if (!player->sliding) {
		float reduction = 1.0f;
		if (keyMap["MOVE_FORWARD"]) {
			if (!player->touchingFloor) {reduction *= 0.5f;}
			newX += playerSpeed * sin(player->viewAngle * constants::TO_RAD) * reduction;
			newY += playerSpeed * cos(player->viewAngle * constants::TO_RAD) * reduction;
		}
		if (keyMap["MOVE_BACKWARD"]) {
			if (!player->touchingFloor) {reduction *= 0.5f;}
			newX -= playerSpeed * sin(player->viewAngle * constants::TO_RAD) * reduction;
			newY -= playerSpeed * cos(player->viewAngle * constants::TO_RAD) * reduction;
		}
		if (keyMap["MOVE_LEFT"]) {
			if (!player->touchingFloor) {reduction *= 0.5f;}
			newX -= playerSpeed * cos((player->viewAngle) * constants::TO_RAD) * reduction;
			newY -= playerSpeed * -sin((player->viewAngle) * constants::TO_RAD) * reduction;
		}
		if (keyMap["MOVE_RIGHT"]) {
			if (!player->touchingFloor) {reduction *= 0.5f;}
			newX += playerSpeed * cos((player->viewAngle) * constants::TO_RAD) * reduction;
			newY += playerSpeed * -sin((player->viewAngle) * constants::TO_RAD) * reduction;
		}
		lateralMovement = glm::vec2(newX, newY);
	}
	if (keyMap["MOVE_JUMP"] && !prevJump) {
		if (player->touchingFloor) {
			player->jumpsUsed++;
			player->velocity.z += playerConfig::JUMP_INIT_SPEED;
			player->position.z += 0.025;
		} else if (player->jumpsUsed < playerConfig::MAX_JUMPS) {
			player->jumpsUsed = playerConfig::MAX_JUMPS;
			if (player->velocity.z < 0.0f) {
				player->velocity.z = playerConfig::JUMP_INIT_SPEED;
			} else {
				const float maxJumpSpeed = playerConfig::JUMP_INIT_SPEED * 2.0f;
				float maxJump = glm::min(player->velocity.z + playerConfig::JUMP_INIT_SPEED, maxJumpSpeed);
				player->velocity.z = maxJump;
			}
		}
	}


	glm::vec2 vAddition = glm::vec2(newX, newY);
	if (length(vAddition) > playerSpeed) {
		vAddition = normalize(vAddition) * playerSpeed;
	}
	glm::vec2 vRight = glm::normalize(glm::vec2(player->velocity.y, -player->velocity.x));

	if (lateralMovement != glm::vec2(0.0f, 0.0f)) {
		glm::vec2 rDir = glm::vec2(
			cos((player->viewAngle) * constants::TO_RAD),
			-sin((player->viewAngle) * constants::TO_RAD)
		);
		float leanDotLR = glm::dot(glm::normalize(lateralMovement), rDir);
		desiredLeanLR = leanDotLR * playerConfig::LATERAL_VIEW_LEAN;

		glm::vec2 fDir = glm::vec2(
			sin((player->viewAngle) * constants::TO_RAD),
			cos((player->viewAngle) * constants::TO_RAD)
		);
		float leanDotFB = glm::dot(glm::normalize(lateralMovement), fDir);
		desiredLeanFB = leanDotFB * playerConfig::LATERAL_VIEW_LEAN;
	} else {
		desiredLeanLR = 0.0f;
		desiredLeanFB = 0.0f;
	}


	player->viewRoll += (desiredLeanLR - player->viewRoll) * 0.125f;
	leanFBCurrent += (desiredLeanFB - leanFBCurrent) * 0.125f;
	float vMoveLean = glm::clamp(player->velocity.z, -1.0f, 1.0f) * playerConfig::LATERAL_VIEW_LEAN;

	player->viewPitch = player->vLook + leanFBCurrent + vMoveLean;
	

	//Occasionally returns NaN somehow.
	glm::vec2 curVelocity = glm::vec2(player->velocity.x, player->velocity.y);
	if (glm::length(curVelocity) > playerConfig::MAX_AIR_SPEED_XY && glm::length(vAddition) > EPSILON) {
		vAddition = vRight * glm::dot(glm::normalize(vAddition), vRight) * playerSpeed;
	}
	player->velocity.x += vAddition.x; player->velocity.y += vAddition.y;



	prevJump = keyMap["MOVE_JUMP"];
	prevSlide = player->sliding;
	touchingFloorCheck = false;




	float playerFootZ = player->position.z - (player->height/2.0f);
	float playerHeadZ = player->position.z + (player->height/2.0f);


	//Vertical Calculations;
	for (int vIndex=0; vIndex<validVisplanes; vIndex++) {
		utils::Visplane vPlane = visplaneData->at(vIndex);

		bool inPlaneXYRange = !(
			(player->position.x + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(vPlane.start.x, vPlane.end.x))
			|| (player->position.x - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(vPlane.start.x, vPlane.end.x))
			|| (player->position.y + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(vPlane.start.y, vPlane.end.y))
			|| (player->position.y - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(vPlane.start.y, vPlane.end.y))
		); // !outOfRange.
		bool abovePlane = player->position.z >= vPlane.height;

		if (inPlaneXYRange) {
			//If ΔZ < 0.42857u then allow player to climb up (stairs, ledge)
			float stepUpZ = vPlane.height - playerFootZ;
			if (stepUpZ <= constants::MAX_STEP_HEIGHT && stepUpZ >= 0.0f) {
				player->position.z += stepUpZ;
				player->velocity.z = 0.0f;
				touchingFloorCheck = true;
			} else if (abovePlane && playerFootZ < vPlane.height) {
				player->position.z = vPlane.height + (player->height/2.0f);
				player->velocity.z = 0.0f;
				touchingFloorCheck = true;
			} else if (!abovePlane && playerHeadZ > vPlane.height) {
				player->position.z = vPlane.height - (player->height/2.0f);
				player->velocity.z = 0.0f;
			}
		}
	}
	player->touchingFloor = touchingFloorCheck;
	if (touchingFloorCheck) {
		player->jumpsUsed = 0;
	}



	//Horizontal Calculations;
	if (utils::configToBool("PHYS_NO_COLLIDE")) {
		glm::vec3 newPos = player->position + glm::vec3(player->velocity.x, player->velocity.y, 0.0f);
		if (isVec3NaN(newPos)) {return;}
		player->position = newPos;
		return;
	}


	for (int wIndex=0; wIndex<validWalls; wIndex++) {
		utils::Wall wall = wallData->at(wIndex);

		if (wall.type == W_TRIGGER) {continue; /* W_TRIGGER can be walked through. */}
		bool playerZCheckWall = !(
			(playerHeadZ < min(wall.start.z, wall.end.z))
			 || (playerFootZ + constants::MAX_STEP_HEIGHT > max(wall.start.z, wall.end.z))
		); //!aboveOrBelow.
		bool touchingWallCheck = circleLineIntersect(
			wall,
			glm::vec2(player->position) + glm::vec2(player->velocity),
			playerConfig::PLAYER_COLLISION_RADIUS
		);

		if (touchingWallCheck && playerZCheckWall) {
			glm::vec2 playerStartDelta = glm::vec2(player->position) - glm::vec2(wall.start);
			glm::vec2 playerEndDelta = glm::vec2(player->position) - glm::vec2(wall.end);

			float playerStartDist = glm::length(playerStartDelta);
			float playerEndDist = glm::length(playerEndDelta);

			if ((playerStartDist < playerConfig::PLAYER_COLLISION_RADIUS) || (playerEndDist < playerConfig::PLAYER_COLLISION_RADIUS)) {
				//Stop the player walking through the start/end of the wall.
				if (playerStartDist < playerEndDist) {
					player->position = glm::vec3(
						glm::vec2(player->position) + glm::normalize(playerStartDelta) * (playerConfig::PLAYER_COLLISION_RADIUS - playerStartDist + 1e-2f),
						player->position.z
					);
				} else {
					player->position = glm::vec3(
						glm::vec2(player->position) + glm::normalize(playerEndDelta) * (playerConfig::PLAYER_COLLISION_RADIUS - playerEndDist + 1e-2f),
						player->position.z
					);
				}
			} else {
				glm::vec2 wallDir = glm::normalize(wall.end - wall.start);
				glm::vec2 wallNormal = glm::vec2(wallDir.y, -wallDir.x);

				glm::vec2 correctedV = wallDir * glm::dot(glm::normalize(glm::vec2(player->velocity.x, player->velocity.y)), wallDir) * playerSpeed;
				player->velocity.x = correctedV.x; player->velocity.y = correctedV.y;
				float distToWall;
				touchingWallCheck = circleLineIntersect(
					wall,
					glm::vec2(player->position) + glm::vec2(player->velocity),
					playerConfig::PLAYER_COLLISION_RADIUS,
					&distToWall
				);

				if (distToWall < playerConfig::PLAYER_COLLISION_RADIUS) {
					float correctionDist = glm::clamp(playerConfig::PLAYER_COLLISION_RADIUS - distToWall, 0.0f, playerConfig::PLAYER_COLLISION_RADIUS);
					float projection = glm::dot(glm::vec2(player->position-wall.start), wallNormal);
					int sign = (projection > 0.0f) ? 1 : -1;
					correctionDist *= sign;
					player->position = glm::vec3(
						glm::vec2(player->position) + wallNormal * correctionDist,
						player->position.z
					);
				}
			}
		}
	}

	for (const utils::Sprite& sprite : *spriteData) {
		if (!(sprite.collision)) {continue;}
		float spriteHeadZ = sprite.position.z + (sprite.height/2.0f);
		float spriteFootZ = sprite.position.z - (sprite.height/2.0f);

		if (playerFootZ > spriteHeadZ || playerHeadZ < spriteFootZ) {continue; /* Above/Below sprite. */}


		glm::vec2 spritePosV2 = glm::vec2(sprite.position.x, sprite.position.y);
		glm::vec2 spriteDir = spritePosV2 - glm::vec2(player->position);
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

		glm::vec2 intersectPoint = glm::vec2(player->position) + glm::vec2(player->velocity.x, player->velocity.y) * Mu;
		glm::vec2 normal = glm::normalize(intersectPoint - spritePosV2);
		glm::vec2 movementAlongNormal = glm::dot(glm::vec2(player->velocity.x, player->velocity.y), normal) * normal;
		glm::vec2 correctedV = glm::vec2(player->velocity.x, player->velocity.y) - movementAlongNormal*0.75f;
		player->velocity.x = correctedV.x; player->velocity.y = correctedV.y;
	}


	//Apply friction.
	if (touchingFloorCheck) {
		if (player->sliding) {
			player->velocity.x *= constants::FLOOR_FRICT_SLIDE_COEFF;
			player->velocity.y *= constants::FLOOR_FRICT_SLIDE_COEFF;			
		} else {
			player->velocity.x *= constants::FLOOR_FRICT_COEFF;
			player->velocity.y *= constants::FLOOR_FRICT_COEFF;
		}
	} else {
		if (player->sliding) {
			player->velocity.x *= constants::AIR_FRICT_SLIDE_COEFF;
			player->velocity.y *= constants::AIR_FRICT_SLIDE_COEFF;			
		} else {
			player->velocity.x *= constants::AIR_FRICT_COEFF;
			player->velocity.y *= constants::AIR_FRICT_COEFF;
		}
		player->velocity.z *= constants::AIR_FRICT_SLIDE_COEFF;
	}

	player->velocity.z -= stageData.gravity / constants::PHYSICS_FREQUENCY;
	player->position += player->velocity;

	if (isVec3NaN(player->position) || isVec3NaN(player->velocity)) {
		*player = playerCopy; //Revert back.
	}
};



void updateSpecials(
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Visplane>* visplaneData,
		utils::Player* player, bool interactKey
	) {


	for (size_t wIndex=0; wIndex<validWalls; wIndex++) {
		utils::Wall wall = wallData->at(wIndex);
		if ((wall.type == W_INVALID) || (wall.type == W_NORMAL)) {continue;}
		bool enabled = *(wall.IOPtr);

		switch(wall.type) {
			case W_TRIGGER: {
				float playerFootZ = player->position.z - (player->height/2.0f);
				float playerHeadZ = player->position.z + (player->height/2.0f);
				bool playerZCheckWall = !(
					(playerHeadZ < (std::min(wall.start.z, wall.end.z)))
					 || (playerFootZ + constants::MAX_STEP_HEIGHT > (std::max(wall.start.z, wall.end.z)))
				); //!aboveOrBelow.
				bool touchingWallCheck = circleLineIntersect(
					wall,
					glm::vec2(player->position) + glm::vec2(player->velocity.x, player->velocity.y),
					playerConfig::PLAYER_COLLISION_RADIUS
				);

				if (touchingWallCheck && playerZCheckWall) {
					*(wall.IOPtr) = true;
				} else {
					*(wall.IOPtr) = false;
				}
				break;
			}

			case W_SWITCH: { //Check for interaction with wall.
				if (interactKey) {
					if (didHitSwitch(player, wIndex, wallData)) {
						*(wall.IOPtr) = !(*(wall.IOPtr));
					}
				}
				//cout << *(wall.IOPtr) << endl;
				break;
			}

			case W_MOVED_FAST: { //Move horizontally (+/- wall direction) quickly.
				specialMotion::applyWallDirectionalMovement(&wall, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case W_MOVED_SLOW: {//Move horizontally (+/- wall direction) slowly.
				specialMotion::applyWallDirectionalMovement(&wall, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			case W_MOVEN_FAST: { //Move horizontally (+/- wall direction) quickly.
				specialMotion::applyWallNormalMovement(&wall, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case W_MOVEN_SLOW: {//Move horizontally (+/- wall direction) slowly.
				specialMotion::applyWallNormalMovement(&wall, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			case W_MOVEZ_FAST: { //Move vertically, quickly.
				specialMotion::applyWallZMovement(&wall, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case W_MOVEZ_SLOW: { //Move vertically, slowly.
				specialMotion::applyWallZMovement(&wall, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			default: {
				break;
			}
		}
		wallData->at(wIndex) = wall;
	}


	for (size_t vIndex=0; vIndex<validVisplanes; vIndex++) {
		utils::Visplane vPlane = visplaneData->at(vIndex);
		if ((vPlane.type == V_INVALID) || (vPlane.type == V_NORMAL)) {continue;}
		bool enabled = *(vPlane.IOPtr);


		bool inPlaneXYRange = !(
			(player->position.x + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(vPlane.start.x, vPlane.end.x))
			|| (player->position.x - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(vPlane.start.x, vPlane.end.x))
			|| (player->position.y + (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) < min(vPlane.start.y, vPlane.end.y))
			|| (player->position.y - (playerConfig::PLAYER_COLLISION_RADIUS/2.0f) > max(vPlane.start.y, vPlane.end.y))
		); // !outOfRange.
		bool abovePlane = player->position.z >= vPlane.height;
		bool planeTouch = false;

		if (inPlaneXYRange) {
			float playerFootZ = player->position.z - (player->height/2.0f);
			float playerHeadZ = player->position.z + (player->height/2.0f);
			//If ΔZ < 0.42857u then allow player to climb up (stairs, ledge)
			float stepUpZ = vPlane.height - playerFootZ;
			if (stepUpZ <= constants::MAX_STEP_HEIGHT && stepUpZ >= 0.0f) {
				planeTouch = true;
			} else if (abovePlane && playerFootZ < vPlane.height) {
				planeTouch = true;
			} else if (!abovePlane && playerHeadZ > vPlane.height) {
				planeTouch = true;
			}
		}


		switch(vPlane.type) {
			case V_TRIGGER: {
				if (planeTouch) {
					*(vPlane.IOPtr) = true;
				} else {
					*(vPlane.IOPtr) = false;
				}
				break;
			}

			case V_MOVEX_FAST: { //Move vertically, quickly.
				specialMotion::applyVisplaneXMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case V_MOVEX_SLOW: { //Move vertically, slowly.
				specialMotion::applyVisplaneXMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			case V_MOVEY_FAST: { //Move vertically, quickly.
				specialMotion::applyVisplaneYMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case V_MOVEY_SLOW: { //Move vertically, slowly.
				specialMotion::applyVisplaneYMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			case V_MOVEZ_FAST: { //Move vertically, quickly.
				specialMotion::applyVisplaneZMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_FAST, enabled);
				break;
			}

			case V_MOVEZ_SLOW: { //Move vertically, slowly.
				specialMotion::applyVisplaneZMovement(&vPlane, constants::SPECIAL_MOVE_SPEED_SLOW, enabled);
				break;
			}

			case V_HURT: {
				if (planeTouch) {
					float hurt = vPlane.data;
					utils::hurtPlayer(player, hurt);
				}
				break;
			}

			default: {
				break;
			}
		}
		visplaneData->at(vIndex) = vPlane;
	}
}

}