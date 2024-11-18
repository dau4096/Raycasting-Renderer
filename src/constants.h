#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

namespace constants {
	constexpr float pi = 3.1415926;
	constexpr float exp = 2.7182818;

	//Constant values for converting between angle units.
	constexpr float toRad = 0.0174533;
	constexpr float toDeg = 57.295780;
}

namespace display {
	constexpr int screenWidth = 640;
	constexpr int screenHeight = 360;

	constexpr float maxRayAngle = 70.0f;
	constexpr float maxRayDistance = 64.0f;

	//Framebuffer roof/floor colours.
	constexpr glm::vec3 topColour(196, 196, 196);
	constexpr glm::vec3 lowColour(128, 128, 128);
}

namespace player {
	constexpr float turnSpeed = 1.0f;
	constexpr float moveSpeed = 0.05f;
}

#endif // CONSTANTS_H