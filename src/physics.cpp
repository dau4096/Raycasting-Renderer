extern "C" {
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/display.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/keyboard.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/rtc.h"
}

#include "utils.h"
#include "raycasting.h"
using namespace std;
using namespace utils;

namespace physics {

bool circleLineIntersect(Wall line, vec2 circlePosition, float radius) {
	vec2 lineDir = SUB(line.end, line.start);  //Direction of the line
	vec2 lineToCircle = SUB(circlePosition, line.start); //Line start to circle


	float t = utils::dot(lineToCircle, lineDir) / utils::dot(lineDir, lineDir);
	t = utils::clamp(t, 0.0f, 1.0f);

	vec2 closestPoint = ADD(line.start, MUL(lineDir, t));
	float distToCircle = utils::length(SUB(circlePosition, closestPoint));

	//In radius?
	return distToCircle <= radius;
}


Player playerMove(Player player, int *key, const Wall (*wallData)[64], int maxWalls) {
	const float minCollisionDist = 0.125f;
	const float moveSpeed = 0.25f;

	float newX = 0.0f, newY = 0.0f;


	//Determine the movement vector based on key presses
	if (*key== KEY_CHAR_8) { //Ahead
		newX =  moveSpeed *  utils::sin(player.viewAngle);
		newY =  moveSpeed *  utils::cos(player.viewAngle);
	} else if (*key == KEY_CHAR_2) { //Behind
		newX = -moveSpeed *  utils::sin(player.viewAngle);
		newY = -moveSpeed *  utils::cos(player.viewAngle);
	} else if (*key == KEY_CHAR_4) { //Left
		newX =  moveSpeed * -utils::cos(player.viewAngle);
		newY =  moveSpeed *  utils::sin(player.viewAngle);
	} else if (*key == KEY_CHAR_6) { //Right
		newX = -moveSpeed * -utils::cos(player.viewAngle);
		newY = -moveSpeed *  utils::sin(player.viewAngle);
	} else {
		return player;
	}

	vec2 movementVector(newX, newY);

	if (true) { //NoCollision option
		player.position = ADD(movementVector, player.position);
		return player;
	}



	//float maxAllowedDistance = utils::length(movementVector); //Unused
	bool collided = false;
	for (int idx = 0; idx < maxWalls; idx++) {
		const Wall& thisWall = *wallData[idx];
		if (circleLineIntersect(thisWall, ADD(player.position, movementVector), minCollisionDist)) {
			collided = true;
		}
	}


	if (!collided) {
		player.position = ADD(movementVector, player.position);
	}

	return player;
};


}