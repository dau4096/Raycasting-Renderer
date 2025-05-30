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
	{"USE_ITEM_PRIMARY", -1},
	{"USE_ITEM_SECONDARY", -1},

	{"META_NEXT_ITEM", -1},
	{"META_PREV_ITEM", -1},
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
inline glm::ivec2 globalScroll;
inline glm::ivec2 currentScreenRes;
inline float freq;


inline std::unordered_map<std::string, std::string> userConfig = {
	{"TURN_SPEED_MOUSE", ""},
	{"TURN_SPEED_KEYBOARD", ""},

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
	{"META_SCREENSHOT_HAS_HUD", ""},

	{"PHYS_NO_COLLIDE", ""},
};


struct StageData {
	std::string skyboxTextureName;
	glm::vec3 sunDirection, sunColour;
	
	float gravity;

	glm::vec3 playerStartPoint;
	float playerStartAngle;
	float playerStartHealth, playerStartEnergy;
	std::array<std::string, playerConfig::MAX_ITEMS_HELD> initialPlayerItems;


	StageData()
		: skyboxTextureName("fallback-skybox"),
		  sunDirection(0.0f, 0.0f, 1.0f), sunColour(1.0f, 1.0f, 1.0f),
		  gravity(0.486),
		  playerStartPoint(0.0f, 0.0f, 0.0f), playerStartAngle(0.0f),
		  playerStartHealth(1.0f), playerStartEnergy(1.0f),
		  initialPlayerItems() {}
};

inline StageData stageData;



struct Item {
	std::string name;
	ItemFunction type;
	std::array<float, 5> data;
	bool active;
	ItemState state;
	std::array<int, 5> textureIDs;

	Item() : name(playerConfig::EMPTY_HAND_ITEM_NAME), type(IFN_INVALID), data(), active(false), state(IS_INVALID), textureIDs{-1, -1, -1, -1, -1} {}

	Item(std::string name, ItemFunction type, std::array<float, 5>& data, std::array<int, 5>& texIDs)
		: name(name), type(type),
		  data(data), state(IS_IDLE),
		  textureIDs(texIDs) {}
};


static inline float getAttributeFromItemData(Item* item, ItemAttr attribute) {
	//Assign attributes based on an item and an enum value, if relevant.
	switch (item->type) {
	case IFN_INVALID: {
		break;
	}
	case IFN_UTILITY: { //Utility item layout.
		switch (attribute) {
		case IA_USE_IS_TOGGLE: {
			return item->data[0];
			break;
		}
		case IA_MAX_USES: {
			return item->data[1];
			break;
		}
		case IA_ENERGY_PER_USE: {
			return item->data[2];
			break;
		}
		case IA_HEALTH_ON_USE: {
			return item->data[3];
			break;
		}
		case IA_ILLUMINATE_SURROUNDINGS: {
			return item->data[4];
			break;
		}
		default: {
			break;
		}
		}
	}

	case IFN_HITSCAN: { //Hitscan item layout.
		switch (attribute) {
		case IA_USE_PER_SECOND: {
			return item->data[0];
			break;
		}
		case IA_ENERGY_PER_USE: {
			return item->data[1];
			break;
		}
		case IA_STRENGTH: {
			return item->data[2];
			break;
		}
		case IA_SHOTS_PER_USE: {
			return item->data[3];
			break;
		}
		case IA_SPREAD_PER_SHOT: {
			return item->data[4];
		}
		default: {
			break;
		}
		}
	}

	case IFN_PROJECTILE: { //Projectile item layout.
		switch (attribute) {
		case IA_USE_PER_SECOND: {
			return item->data[0];
			break;
		}
		case IA_MAX_USES: {
			return item->data[1];
			break;
		}
		case IA_ENERGY_PER_USE: {
			return item->data[2];
			break;
		}
		case IA_STRENGTH: {
			return item->data[3];
			break;
		}
		case IA_PROJECTILE_SPEED: {
			return item->data[4];
			break;
		}
		default: {
			break;
		}
		}
	}

	case IFN_MELEE: { //Melee item layout.
		switch (attribute) {
		case IA_USE_PER_SECOND: {
			return item->data[0];
			break;
		}
		case IA_MAX_USES: {
			return item->data[1];
			break;
		}
		case IA_ENERGY_PER_USE: {
			return item->data[2];
			break;
		}
		case IA_STRENGTH: {
			return item->data[3];
			break;
		}
		default: {
			break;
		}
		}
	}

	default: {
		break;
	}
	}

	return -1.0f;
}

inline std::unordered_map<std::string, Item> itemData;




