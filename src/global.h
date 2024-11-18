#ifndef GLOBAL_H
#define GLOBAL_H

#include "includes.h"
#include "utils.h"
#include <vector>
#include <unordered_map>
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>


//Shared global values
namespace global {
	extern glm::vec2 playerPosition;
	extern float playerViewAngle;

	extern std::unordered_map<int, bool> keyMap;
	
	//Warning; Compiler error, can't be bothered to fix it currently.
	utils::Line levelData[256];
}

#endif