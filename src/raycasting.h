#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "utils.h"

namespace raycasting {
	
	vec2 castRay(Ray ray, Wall wall);
	void renderScene(Player player, const Wall (*wallData)[64], int maxWalls);
}

#endif