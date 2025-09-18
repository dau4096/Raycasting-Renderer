#ifndef LOADER_H
#define LOADER_H

#include "includes.h"
#include "global.h"
#include "utils.h"


namespace loader {

	int assignTexture(std::string textureStr);

	void loadStage(
		const std::string& stageName, structs::Player* player,
		structs::DataSet* physicsData,
		std::vector<utils::LogicGate>* logicGates
	);


	void loadBindings();

}


#endif