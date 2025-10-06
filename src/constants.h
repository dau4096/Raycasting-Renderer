#pragma once

#include "includes.h"
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

enum Event {
	E_NONE, E_DEAD,
	E_HURT, E_HEAL,
	E_NEW_IH, E_ENERGY,
	E_TELEPORT, E_RESPAWN
};

enum ItemFloor {
	IF_HEALTH_SMALL, IF_HEALTH_LARGE,
	IF_ENERGY_SMALL, IF_ENERGY_LARGE,
	IF_WEAPON
};

enum ItemHeld {
	IH_NONE
};

enum VisplaneType {
	V_INVALID, V_NORMAL,
	V_TRIGGER,
	V_MOVEX_FAST, V_MOVEX_SLOW,
	V_MOVEY_FAST, V_MOVEY_SLOW,
	V_MOVEZ_FAST, V_MOVEZ_SLOW,
	V_HURT, V_PASSTHROUGH,
	V_NODRAW, V_TELEPORT,
	V_CONVEY, V_LIGHTBLOCKER,
	V_PORTAL
};

enum WallType {
	W_INVALID, W_NORMAL,
	W_TRIGGER,
	W_MOVED_FAST, W_MOVED_SLOW,
	W_MOVEN_FAST, W_MOVEN_SLOW,
	W_MOVEZ_FAST, W_MOVEZ_SLOW,
	W_SWITCH, W_PASSTHROUGH,
	W_DOORZ, W_DOORSWING,
	W_NODRAW, W_LIGHTBLOCKER,
	W_PORTAL
};

enum CuboidType {
	C_INVALID, C_NORMAL,
	C_MOVEX_FAST, C_MOVEX_SLOW,
	C_MOVEY_FAST, C_MOVEY_SLOW,
	C_MOVEZ_FAST, C_MOVEZ_SLOW,
	C_PASSTHROUGH, C_NODRAW
};

enum DisplacementType {
	D_INVALID, D_NORMAL
};

enum SpriteType {
	SPR_INVALID,
	SPR_DECO, SPR_LIGHT,
	SPR_PHYSICS, SPR_PARTICLE
};

enum LightingType {
	LIGHT_NONE, LIGHT_SURFACE, LIGHT_DYNAMIC
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


enum ParticleType {
	P_NONE,
	P_DUST,
	P_ENERGY,
	P_HURT,
	P_EXPLODE,
	P_DUST_NOFALL
};



inline const std::unordered_map<std::string, int> keyNameToGLFW = {
	//Main Keys
    {"KEY_SPACE", GLFW_KEY_SPACE},
    {"KEY_APOSTROPHE", GLFW_KEY_APOSTROPHE},
    {"KEY_COMMA", GLFW_KEY_COMMA},
    {"KEY_MINUS", GLFW_KEY_MINUS},
    {"KEY_PERIOD", GLFW_KEY_PERIOD},
    {"KEY_SLASH", GLFW_KEY_SLASH},
    {"KEY_0", GLFW_KEY_0},
    {"KEY_1", GLFW_KEY_1},
    {"KEY_2", GLFW_KEY_2},
    {"KEY_3", GLFW_KEY_3},
    {"KEY_4", GLFW_KEY_4},
    {"KEY_5", GLFW_KEY_5},
    {"KEY_6", GLFW_KEY_6},
    {"KEY_7", GLFW_KEY_7},
    {"KEY_8", GLFW_KEY_8},
    {"KEY_9", GLFW_KEY_9},
    {"KEY_SEMICOLON", GLFW_KEY_SEMICOLON},
    {"KEY_EQUAL", GLFW_KEY_EQUAL},
    {"KEY_A", GLFW_KEY_A},
    {"KEY_B", GLFW_KEY_B},
    {"KEY_C", GLFW_KEY_C},
    {"KEY_D", GLFW_KEY_D},
    {"KEY_E", GLFW_KEY_E},
    {"KEY_F", GLFW_KEY_F},
    {"KEY_G", GLFW_KEY_G},
    {"KEY_H", GLFW_KEY_H},
    {"KEY_I", GLFW_KEY_I},
    {"KEY_J", GLFW_KEY_J},
    {"KEY_K", GLFW_KEY_K},
    {"KEY_L", GLFW_KEY_L},
    {"KEY_M", GLFW_KEY_M},
    {"KEY_N", GLFW_KEY_N},
    {"KEY_O", GLFW_KEY_O},
    {"KEY_P", GLFW_KEY_P},
    {"KEY_Q", GLFW_KEY_Q},
    {"KEY_R", GLFW_KEY_R},
    {"KEY_S", GLFW_KEY_S},
    {"KEY_T", GLFW_KEY_T},
    {"KEY_U", GLFW_KEY_U},
    {"KEY_V", GLFW_KEY_V},
    {"KEY_W", GLFW_KEY_W},
    {"KEY_X", GLFW_KEY_X},
    {"KEY_Y", GLFW_KEY_Y},
    {"KEY_Z", GLFW_KEY_Z},
    {"KEY_LEFT_BRACKET", GLFW_KEY_LEFT_BRACKET},
    {"KEY_BACKSLASH", GLFW_KEY_BACKSLASH},
    {"KEY_RIGHT_BRACKET", GLFW_KEY_RIGHT_BRACKET},
    {"KEY_GRAVE_ACCENT", GLFW_KEY_GRAVE_ACCENT},
    {"KEY_WORLD_1", GLFW_KEY_WORLD_1},
    {"KEY_WORLD_2", GLFW_KEY_WORLD_2},


    //Functional Keys
    {"KEY_ESCAPE", GLFW_KEY_ESCAPE},
    {"KEY_ENTER", GLFW_KEY_ENTER},
    {"KEY_TAB", GLFW_KEY_TAB},
    {"KEY_BACKSPACE", GLFW_KEY_BACKSPACE},
    {"KEY_INSERT", GLFW_KEY_INSERT},
    {"KEY_DELETE", GLFW_KEY_DELETE},
    {"KEY_RIGHT", GLFW_KEY_RIGHT},
    {"KEY_LEFT", GLFW_KEY_LEFT},
    {"KEY_DOWN", GLFW_KEY_DOWN},
    {"KEY_UP", GLFW_KEY_UP},
    {"KEY_PAGE_UP", GLFW_KEY_PAGE_UP},
    {"KEY_PAGE_DOWN", GLFW_KEY_PAGE_DOWN},
    {"KEY_HOME", GLFW_KEY_HOME},
    {"KEY_END", GLFW_KEY_END},
    {"KEY_CAPS_LOCK", GLFW_KEY_CAPS_LOCK},
    {"KEY_SCROLL_LOCK", GLFW_KEY_SCROLL_LOCK},
    {"KEY_NUM_LOCK", GLFW_KEY_NUM_LOCK},
    {"KEY_PRINT_SCREEN", GLFW_KEY_PRINT_SCREEN},
    {"KEY_PAUSE", GLFW_KEY_PAUSE},
    {"KEY_F1", GLFW_KEY_F1},
    {"KEY_F2", GLFW_KEY_F2},
    {"KEY_F3", GLFW_KEY_F3},
    {"KEY_F4", GLFW_KEY_F4},
    {"KEY_F5", GLFW_KEY_F5},
    {"KEY_F6", GLFW_KEY_F6},
    {"KEY_F7", GLFW_KEY_F7},
    {"KEY_F8", GLFW_KEY_F8},
    {"KEY_F9", GLFW_KEY_F9},
    {"KEY_F10", GLFW_KEY_F10},
    {"KEY_F11", GLFW_KEY_F11},
    {"KEY_F12", GLFW_KEY_F12},


    //Keypad Keys
    {"KEY_KP_0", GLFW_KEY_KP_0},
    {"KEY_KP_1", GLFW_KEY_KP_1},
    {"KEY_KP_2", GLFW_KEY_KP_2},
    {"KEY_KP_3", GLFW_KEY_KP_3},
    {"KEY_KP_4", GLFW_KEY_KP_4},
    {"KEY_KP_5", GLFW_KEY_KP_5},
    {"KEY_KP_6", GLFW_KEY_KP_6},
    {"KEY_KP_7", GLFW_KEY_KP_7},
    {"KEY_KP_8", GLFW_KEY_KP_8},
    {"KEY_KP_9", GLFW_KEY_KP_9},
    {"KEY_KP_DECIMAL", GLFW_KEY_KP_DECIMAL},
    {"KEY_KP_DIVIDE", GLFW_KEY_KP_DIVIDE},
    {"KEY_KP_MULTIPLY", GLFW_KEY_KP_MULTIPLY},
    {"KEY_KP_SUBTRACT", GLFW_KEY_KP_SUBTRACT},
    {"KEY_KP_ADD", GLFW_KEY_KP_ADD},
    {"KEY_KP_ENTER", GLFW_KEY_KP_ENTER},
    {"KEY_KP_EQUAL", GLFW_KEY_KP_EQUAL},


    //Modifier Keys
    {"KEY_LEFT_SHIFT", GLFW_KEY_LEFT_SHIFT},
    {"KEY_LEFT_CONTROL", GLFW_KEY_LEFT_CONTROL},
    {"KEY_LEFT_ALT", GLFW_KEY_LEFT_ALT},
    {"KEY_LEFT_SUPER", GLFW_KEY_LEFT_SUPER},
    {"KEY_RIGHT_SHIFT", GLFW_KEY_RIGHT_SHIFT},
    {"KEY_RIGHT_CONTROL", GLFW_KEY_RIGHT_CONTROL},
    {"KEY_RIGHT_ALT", GLFW_KEY_RIGHT_ALT},
    {"KEY_RIGHT_SUPER", GLFW_KEY_RIGHT_SUPER},
    {"KEY_MENU", GLFW_KEY_MENU}
};




namespace constants {
	static bool C_TRUE = true;
	static bool C_FALSE = false;


	//Mathematical Constants
	constexpr float PI = 3.141592f;
	constexpr float PI2 = PI * 2.0f;
	constexpr float EXP = 2.718281f;
	constexpr float INF = std::numeric_limits<float>::infinity();

	constexpr float TO_RAD = 0.017453f;
	constexpr float TO_DEG = 57.29577f;



	//Sim Constants
	constexpr float PHYSICS_FREQUENCY = 60.0f;
	constexpr float GRAVITY_ACCEL = 0.486f;
	constexpr float FLOOR_FRICT_COEFF = 0.75f;
	constexpr float FLOOR_FRICT_SLIDE_COEFF = 0.975f;
	constexpr float AIR_FRICT_COEFF = 0.975f;
	constexpr float AIR_FRICT_SLIDE_COEFF = 0.9975f;
	constexpr float MAX_STEP_HEIGHT = 0.42857f;
	constexpr float DOOR_OPEN_TIME_TICKS = 2.0f * PHYSICS_FREQUENCY;


	//Invalid returns for vectors and floats.
	constexpr float INVALID = 1e30f;
	constexpr glm::vec2 INVALIDv2 = glm::vec2(INVALID, INVALID);
	constexpr glm::vec3 INVALIDv3 = glm::vec3(INVALID, INVALID, INVALID);
	constexpr glm::vec4 INVALIDv4 = glm::vec4(INVALID, INVALID, INVALID, INVALID);


	//Maximum quantities of each type.
	constexpr size_t MAX_FLAGS = 256;
	constexpr size_t MAX_VERTEX_BYTES = 16384;
	constexpr size_t MAX_INDEX_BYTES = 16384;
	constexpr size_t MAX_ROLLING_VALUE_QUALITY = 64;
	constexpr size_t MAX_SPRITE_PARTICLES = 2048;

	constexpr float PARTICLE_LIFETIME_FRAMES = 2.0f * PHYSICS_FREQUENCY;
	constexpr float SPECIAL_MOVE_SPEED_SLOW = 0.025;
	constexpr float SPECIAL_MOVE_SPEED_FAST = 0.075;
}

namespace display {
	//Resolutions
	constexpr glm::ivec2 INITIAL_SCREEN_RESOLUTION = glm::ivec2(960, 540);
	constexpr glm::ivec2 UI_RESOLUTION = glm::ivec2(960, 540);
	constexpr float PREMADE_SHADOW_MAPS_TEXEL_SIZE = 0.01f;


	//Texture Standardisation
	constexpr glm::ivec2 SKYBOX_RESOLUTION = glm::ivec2(512, 256);
	constexpr glm::ivec2 TEXTURE_RESOLUTION = glm::ivec2(128, 128);
	constexpr size_t TEXTURE_ARRAY_MAX_LAYERS = 64;
	constexpr const char* FALLBACK_TEXTURE_PATH = "src/textures-env/fallback.png";
	constexpr const char* FALLBACK_NORMAL_PATH = "src/textures-env/fallback.normal.png";
	constexpr const char* FALLBACK_SKYBOX_PATH = "src/textures-env/fallback-skybox.png";


	//Rendering Assorted
	constexpr float ZOOM_MULT = 3.0f;
	constexpr size_t MAX_TEXTOBJECT_CHARACTERS = 64;
	constexpr float SHADOWMAP_SCALING = 0.01f;
}

namespace initial {
	//Textures
	constexpr const char* FALLBACK_TEXTURE_NAME = "fallback";
	constexpr const char* FALLBACK_SKYBOX_NAME = "fallback-skybox";
	constexpr glm::vec2 TEXTURE_SCALE = glm::vec2(1.0f, 1.0f);
	constexpr glm::vec3 TEXTURE_OFFSET = glm::vec3(0.0f, 0.0f, 0.0f);


	//Sun
	constexpr glm::vec3 SUN_DIRECTION = glm::vec3(0.0f, 0.0f, 1.0f);
	constexpr float SUN_INTENSITY = 1.75f;
	constexpr glm::vec3 SUN_COLOUR = glm::vec3(1.0f, 1.0f, 1.0f);


	//Physics
	constexpr float GRAVITY_ACCEL = 0.486f;
	constexpr float KILL_PLANE_Z = -16.0f;


	//Player
	constexpr glm::vec3 PLAYER_START_POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
	constexpr float PLAYER_START_VANGLE = 0.0f;
}

namespace playerConfig {
	//Physics speed values
	constexpr float MOVE_SPEED_BASE = 0.0375f;
	constexpr float MOVE_SPEED_CROUCH_MULT = 0.5f;
	constexpr float MOVE_SPEED_RUN_MULT = 2.0f;
	constexpr float MOVE_SPEED_SLIDE_ADD = 0.5f;
	constexpr float SLIDE_THRESHOLD = MOVE_SPEED_BASE * 1.125f;
	constexpr float JUMP_INIT_SPEED = 0.25f;
	constexpr float MAX_AIR_SPEED_XY = MOVE_SPEED_BASE * MOVE_SPEED_RUN_MULT * 2.5f;
	constexpr int MAX_JUMPS = 2; //Double jumps allowed.


	//Physics Collision Values
	constexpr float PLAYER_COLLISION_RADIUS = 0.25f;
	constexpr float PLAYER_COLLISION_HEIGHT_STAND = 1.75f;
	constexpr float PLAYER_COLLISION_HEIGHT_CROUCH = 0.875f;
	constexpr float PLAYER_INTERACT_RAY_DIST = 2.0f;


	//Player Initial Values
	constexpr float LATERAL_VIEW_LEAN = 2.5f * constants::TO_RAD;
	constexpr int PLAYER_MAX_HEALTH = 128;
	constexpr int PLAYER_MAX_ENERGY = 64;

	constexpr int HEAL_SMALL = 32;
	constexpr int HEAL_LARGE = 96;
	constexpr int ENERGY_SMALL = 16;
	constexpr int ENERGY_LARGE = 32;
}

namespace dev {
	//Assorted DEV/DEBUG constants
	constexpr bool SHOW_PHYSICS_TICKRATE = false;
	constexpr bool SHOW_PHYSICS_DT = false;
	constexpr bool PAUSE_ON_OPENGL_ERROR = true;
}