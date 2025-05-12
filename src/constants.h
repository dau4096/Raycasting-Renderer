#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

enum Event {
	E_NONE, E_DEAD,
	E_HURT, E_HEAL,
	E_NEW_IH, E_ENERGY
};

enum ItemFloor {
	IF_HEALTH_SMALL, IF_HEALTH_LARGE,
	IF_ENERGY_SMALL, IF_ENERGY_LARGE,
	IF_WEAPON
};

enum ItemHeld {
	IH_NONE
};

enum SpriteType {
	SPR_INVALID,
	SPR_DECO,
	SPR_LIGHT
};

enum WallType {
	W_INVALID, W_NORMAL,
	W_TRIGGER,
	W_MOVEV_FAST, W_MOVEV_SLOW,
	W_MOVEH_FAST, W_MOVEH_SLOW,
	W_SWITCH
};

enum VisplaneType {
	V_INVALID, V_NORMAL,
	V_TRIGGER,
	V_MOVEV_FAST, V_MOVEV_SLOW,
	V_HURT
};

enum LogicInput {
	L_FALSE = 0,
	L_TRUE = 1,

	//Only used for W_SWITCH, W_TRIGGER and V_TRIGGER.
	L_TOGGLE, //Successive presses turn it on, then off, then on etc.
	L_PERMA, //Pressed once, stays on permanently after.
	L_PUSH //Only on while being pressed.
};


enum GateType {
	G_INVALID,		// N/A
	G_PASSTHROUGH,	// =
	G_AND,			// &
	G_OR, 			// |
	G_NOT,			// ~
	G_XOR,			// ^
	G_LATCH,		// 2 inputs, turns on with input A and off with input B.
	G_PULSE,		// 1 input, turns on for 1 frame of the input, then off after.
	G_TOGGLE		// 1 input, turns on and off with that input.
};



namespace constants {
	static int TRUE = 1;
	static int FALSE = 0;


	//Mathematical Constants
	constexpr float PI = 3.14159265358979f;
	constexpr float EXP = 2.71828182845905f;

	constexpr float TO_RAD = 0.01745329251994f;
	constexpr float TO_DEG = 57.2957795130824f;


	//Texture Standardisation
	constexpr glm::ivec2 TEXTURE_RESOLUTION = glm::ivec2(128, 128);
	constexpr int TEXTURE_ARRAY_MAX_LAYERS = 32;


	//Physics/Rendering Frequency/dt
	constexpr int HZ = 45;
	constexpr double DT = 1.0d/HZ;


	//Sim Constants
	constexpr float GRAVITY_ACCEL = 2.5e-4f;
	constexpr float FLOOR_FRICT_COEFF = 0.75f;
	constexpr float FLOOR_FRICT_SLIDE_COEFF = 0.975f;
	constexpr float AIR_FRICT_COEFF = 0.975f;
	constexpr float AIR_FRICT_SLIDE_COEFF = 0.9975f;
	constexpr float KILL_PLANE_HEIGHT = -16.0f;
	constexpr float MAX_STEP_HEIGHT = 0.42857f;


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
	constexpr int MAX_GATES = 32;
	constexpr int MAX_FLAGS = 128;

	constexpr float SPECIAL_MOVE_SPEED_SLOW = 0.025;
	constexpr float SPECIAL_MOVE_SPEED_FAST = 0.075;
}

namespace display {
	//Resolutions
	constexpr glm::ivec2 SCREEN_RESOLUTION = glm::ivec2(640, 400);
	constexpr glm::ivec2 RENDER_RESOLUTION = glm::ivec2(480, 270);


	//Rendering Assorted
	constexpr float ZOOM_MULT = 3.0f;
	constexpr float FOV = 70.0f;
	constexpr float MAX_RAY_ANGLE = FOV / 2.0f;
	constexpr float MAX_RAY_DIST = 128.0f;
}

namespace playerConfig {
	constexpr std::string STAGE_NAME = "dev";

	//Preference Speeds
	constexpr float TURN_SPEED_KEYB = 1.0f;
	constexpr float TURN_SPEED_CURS = 0.25f;

	constexpr float MOVE_SPEED_BASE = 0.05f;
	constexpr float MOVE_SPEED_CROUCH_MULT = 0.5f;
	constexpr float MOVE_SPEED_RUN_MULT = 2.0f;
	constexpr float MOVE_SPEED_SLIDE_ADD = 0.125f;
	constexpr float SLIDE_THRESHOLD = MOVE_SPEED_BASE * 1.5f;
	constexpr float JUMP_INIT_SPEED = 0.25f;
	constexpr float MAX_AIR_SPEED_XY = MOVE_SPEED_BASE * MOVE_SPEED_RUN_MULT * 2.0f;
	constexpr int MAX_JUMPS = 2; //Double jumps allowed.


	//Physics Collision Values
	constexpr float PLAYER_COLLISION_RADIUS = 0.125f;
	constexpr float PLAYER_COLLISION_HEIGHT_STAND = 1.75f;
	constexpr float PLAYER_COLLISION_HEIGHT_CROUCH = 1.0f;
	constexpr float PLAYER_INTERACT_RAY_DIST = 2.0f;


	//Player Initial Values
	constexpr glm::vec3 PLAYER_START_POSITION = glm::vec3(-2.5f, -2.5f, 1.0f);
	constexpr float PLAYER_START_ANGLE = -45.0f;
	constexpr float LATERAL_VIEW_LEAN = 2.5f;
	constexpr int PLAYER_MAX_HEALTH = 128;
	constexpr int PLAYER_MAX_ENERGY = 64;

	constexpr int HEAL_SMALL = 32;
	constexpr int HEAL_LARGE = 96;
	constexpr int ENERGY_SMALL = 16;
	constexpr int ENERGY_LARGE = 32;
}

namespace dev {
	//Assorted DEV/DEBUG constants
	constexpr int DRAW_UV = 0;
	constexpr int NO_COLLIDE = 0;
	constexpr int SHOW_FREQ = 1;
	constexpr int NO_INTERFACE = 0;
	constexpr int VIEW_BOB_DISABLE = 1;
	constexpr int VIEW_LEAN_DISABLE = 1;
	constexpr int LOCK_VLOOK = 1;
}

#endif // CONSTANTS_H