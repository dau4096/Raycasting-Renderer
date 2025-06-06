#pragma once
#include "includes.h"
#include "constants.h"

inline std::unordered_map<std::string, int> userBindings = {
	{"MOVE_FORWARD", -1},
	{"MOVE_BACKWARD", -1},
	{"MOVE_LEFT", -1},
	{"MOVE_RIGHT", -1},
	{"MOVE_JUMP", -1},
	{"MOVE_CROUCH", -1},
	{"MOVE_SPRINT", -1},

	{"USE_INTERACT", -1},
	{"USE_HEADLAMP", -1},
	{"USE_VIEWZOOM", -1},

	{"META_SCREENSHOT", -1},
	{"META_RELOAD_STAGE", -1},
	{"META_RELOAD_ENV", -1},
	{"META_EXIT", -1},
	{"META_FREECURSOR", -1},
};


inline std::unordered_map<std::string, bool> keyMap = []() {
	std::unordered_map<std::string, bool> tmp;
	for (const auto& pair : userBindings) {
		tmp[pair.first] = false;
	}
	return tmp;
}();


inline std::unordered_map<std::string, std::string> userConfig = {
	{"TURN_SPEED_MOUSE", ""},
	{"TURN_SPEED_KEYBOARD", ""},

	{"VIEW_RENDER_RESOLUTION_QUALITY", ""},
	{"VIEW_FOV", ""},
	{"VIEW_MAX_RAY_DIST", ""},
	{"VIEW_SHOW_HUD", ""},
	{"VIEW_DRAW_UV", ""},
	{"VIEW_BOB", ""},
	{"VIEW_LEAN", ""},
	{"VIEW_VLOOK", ""},
	{"VIEW_SMOOTHING", ""},
	{"VIEW_ANTIALIAS_LEVEL", ""},
	{"VIEW_LUMINANCE_QUANTISATION", ""},
	{"VIEW_MAX_FREQ", ""},
	{"VIEW_NO_INTERFACE", ""},

	{"META_DRAW_UV", ""},
	{"META_SHOW_FREQ_UI", ""},
	{"META_SHOW_FREQ_CONSOLE", ""},
	{"META_SHOW_CONSOLE", ""},
	{"META_STAGE_NAME", ""},

	{"PHYS_NO_COLLIDE", ""},
};


struct StageData {
	std::string skyboxTextureName;
	glm::vec3 sunDirection, sunColour;
	
	float gravity;
	float killPlaneZ;

	glm::vec3 playerStartPoint;
	float playerStartAngle;
	float playerStartHealth, playerStartEnergy;


	StageData()
		: skyboxTextureName("fallback-skybox"),
		  sunDirection(0.0f, 0.0f, 1.0f), sunColour(1.0f, 1.0f, 1.0f),
		  gravity(0.486), killPlaneZ(-64.0f),
		  playerStartPoint(0.0f, 0.0f, 0.0f), playerStartAngle(0.0f),
		  playerStartHealth(1.0f), playerStartEnergy(1.0f) {}
};

inline StageData stageData;


inline int validVisplanes;
inline int validWalls;
inline int validSprites;
inline int validLights;
inline int validTextObjects;
inline int validGates;

inline glm::ivec2 currentWindowResolution;
inline glm::ivec2 desiredRenderResolution;
inline glm::ivec2 currentRenderResolution;