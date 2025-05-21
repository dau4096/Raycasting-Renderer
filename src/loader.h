#ifndef LOADER_H
#define LOADER_H

#include "includes.h"
#include "global.h"
#include "utils.h"
#include <array>


namespace loader {

	void loadStage(
		const std::string& stageName,  utils::Player* player,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData,
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Light, constants::MAX_LIGHTS>* lightData,
		std::array<utils::LogicGate, constants::MAX_GATES>* logicGates,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	);


	void loadBindings();

}


#endif