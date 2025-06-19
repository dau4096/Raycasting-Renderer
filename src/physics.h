#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"

namespace physics {

	bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius);

	void playerMove(
		utils::Player *player,
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Sprite>* spriteData,
		std::vector<utils::Visplane>* visplaneData
	);

	void updateSpecials(
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Visplane>* visplaneData,
		utils::Player *player, bool interactKey
	);
}

#endif