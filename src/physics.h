#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"
//Function names and args here.
//I.e. int add(int a, int b);
namespace physics {

	utils::Player playerMove(utils::Player player, std::unordered_map<int, bool> keyMap, const std::array<utils::Wall, 256>* wallData);

}

#endif