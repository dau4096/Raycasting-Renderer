#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

namespace constants {
	constexpr float pi = 3.1415926f;
	constexpr float exp = 2.7182818f;

	//Constant values for converting between angle units.
	constexpr float toRad = 0.0174533f;
	constexpr float toDeg = 57.295780f;

	constexpr int textureWidth = 128;
	constexpr int textureHeight = 128;
}

namespace display {
	constexpr int screenWidth = 640;
	constexpr int screenHeight = 360;

	constexpr int maxFPS = 75;

	constexpr float zoomFactor = 3.0f;
	constexpr float maxRayAngle = 35.0f;
	constexpr float maxRayDistance = 64.0f;

	//Framebuffer roof/floor colours.
	constexpr int topIndex = 3;
	constexpr int lowIndex = 1;
	constexpr glm::vec3 topColour(135, 206, 235);
	constexpr glm::vec3 lowColour(128, 128, 128);
	constexpr float dimmingStrength = 1.0f;
}

namespace playerConfig {
	constexpr float turnSpeedKB = 1.0f;
	constexpr float turnSpeedCursor = 0.25f;
	constexpr float moveSpeed = 0.05f;
	constexpr float runMultiplier = 2.0f;
	constexpr float minCollisionDist = 0.125f;

	constexpr glm::vec2 playerStartPos = glm::vec2(-2.5f, -2.5f);
	constexpr float playerStartAngle = -45.0f;
}

namespace dev {
	constexpr int drawUV = 0;
	constexpr int noCollis = 0;
	constexpr int printFPS = 0;
	constexpr int noUI = 1;
}

#endif // CONSTANTS_H