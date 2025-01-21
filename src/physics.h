#ifndef PHYSICS_H
#define PHYSICS_H

#include "utils.h"
//Function names and args here.
//I.e. int add(int a, int b);
namespace physics {

	Player playerMove(Player player, int *key, const Wall (*wallData)[64], int maxWalls);

}

#endif