#ifndef LOADER_H
#define LOADER_H

#include "includes.h"
#include "global.h"
#include "utils.h"


namespace loader {

	void loadStage(
		const std::string& stageName, utils::Player* player,
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Displacement>* displacementData,
		std::vector<utils::Sprite>* spriteData,
		std::vector<utils::Light>* lightData,
		std::vector<utils::TextObject>* textObjectData,
		std::vector<utils::LogicGate>* logicGates,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	);


	void loadBindings();

}


#endif