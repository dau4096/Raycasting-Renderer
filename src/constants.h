#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <array>

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

	//Framebuffer roof/floor colours.
	constexpr std::array<int, 3> topColour = {196, 196, 196};
	constexpr std::array<int, 3> lowColour = {128, 128, 128};
}

namespace player {
	constexpr float turnSpeed = 0.1f;
	constexpr float moveSpeed = 0.1f;
}

#endif // CONSTANTS_H