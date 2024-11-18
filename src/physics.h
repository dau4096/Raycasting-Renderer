#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"
//Function names and args here.
//I.e. int add(int a, int b);
namespace physics {

	glm::vec2 playerMove(glm::vec2 playerPosition, float playerViewAngle, std::unordered_map<int, bool> keyMap);
	bool checkCollision();
	float angleClamp(float value);

}

#endif