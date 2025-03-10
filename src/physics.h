#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"
//Function names and args here.
//I.e. int add(int a, int b);
namespace physics {

	bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius);
	utils::Player playerMove(
		utils::Player player,
		std::unordered_map<int, bool> keyMap,
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData
	);

}

#endif