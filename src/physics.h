#ifndef PHYSICS_H
#define PHYSICS_H

#include "includes.h"
#include "utils.h"

namespace physics {

	bool circleLineIntersect(structs::Wall line, glm::vec2 circlePosition, float radius);

	void playerMovement();

	void updatePhysicsObjects();

	void updateSpecials(bool interactKey);
}

#endif