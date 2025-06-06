#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"

namespace physics {

	bool circleLineIntersect(utils::Wall line, glm::vec2 circlePosition, float radius);

	void playerMove(
		utils::Player *player, float freq,
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData
	);

	void updateSpecials(
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData,
		utils::Player *player, float freq, bool interactKey
	);
}

#endif