#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

namespace constants {
	//Mathematical Constants
	constexpr float PI = 3.14159265358979f;
	constexpr float EXP = 2.71828182845905f;

	constexpr float TO_RAD = 0.01745329251994f;
	constexpr float TO_DEG = 57.2957795130824f;


	//Texture Standardisation
	constexpr glm::ivec2 TEXTURE_RESOLUTION = glm::ivec2(128, 128);
	constexpr int TEXTURE_ARRAY_MAX_LAYERS = 32;


	//Physics/Rendering Frequency/dt
	constexpr int HZ = 20;
	constexpr float DT = 1/HZ;


	//Invalid returns for vectors and floats.
	constexpr float INVALID = 1e30f;
	constexpr glm::vec2 INVALIDv2 = glm::vec2(INVALID, INVALID);
	constexpr glm::vec3 INVALIDv3 = glm::vec3(INVALID, INVALID, INVALID);
	constexpr glm::vec4 INVALIDv4 = glm::vec4(INVALID, INVALID, INVALID, INVALID);


	//Maximum quantities of each type.
	constexpr int MAX_VISPLANES = 64;
	constexpr int MAX_WALLS = 256;
	constexpr int MAX_SPRITES = 32;
	constexpr int MAX_LIGHTS = 64;
}

namespace display {
	//Resolutions
	constexpr glm::ivec2 SCREEN_RESOLUTION = glm::ivec2(640, 400);
	constexpr glm::ivec2 RENDER_RESOLUTION = glm::ivec2(480, 270);


	//Rendering Assorted
	constexpr float ZOOM_MULT = 3.0f;
	constexpr float FOV = 70.0f;
	constexpr float MAX_RAY_ANGLE = FOV / 2.0f;
	constexpr float MAX_RAY_DIST = 64.0f;
}

namespace playerConfig {
	//Preference Speeds
	constexpr float TURN_SPEED_KEYB = 1.0f;
	constexpr float TURN_SPEED_CURS = 0.25f;

	constexpr float MOVE_SPEED_BASE = 0.05f;
	constexpr float MOVE_SPEED_RUN_MULT = 2.0f;


	//Physics Collision Values
	constexpr float PLAYER_COLLISION_RADIUS = 0.125f;
	constexpr float PLAYER_COLLISION_HEIGHT = 1.8f;


	//Player Initial Values
	constexpr glm::vec3 PLAYER_START_POSITION = glm::vec3(-2.5f, -2.5f, 0.0f);
	constexpr float PLAYER_START_ANGLE = -45.0f;
}

namespace dev {
	//Assorted DEV/DEBUG constants
	constexpr int DRAW_UV = 0;
	constexpr int NO_COLLIDE = 0;
	constexpr int SHOW_FREQ = 0;
	constexpr int NO_INTERFACE = 1;
}

#endif // CONSTANTS_H