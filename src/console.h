#ifndef CONSOLE_H
#define CONSOLE_H


#include "includes.h"
#include "constants.h"
#include "global.h"
#include "utils.h"
#include "loader.h"
#include "graphics.h"
#include "physics.h"



namespace stage {

inline void reload(const bool resetPlayer=false) {
	if (resetPlayer) {
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &player,
			physicsData, &logicGates
		);
	} else {
		structs::Player tmpPlayer;
		loader::loadStage(
			userConfig["META_STAGE_NAME"], &tmpPlayer,
			physicsData, &logicGates
		);
	}

	graphics::prepareOpenGL();
	{
		std::lock_guard<std::mutex> lock(stateSwapMutex);
		*graphicsData = *physicsData;
	}
}

}


std::vector<std::string> split(const std::string& line) {
	//Split string into "words".
    std::istringstream iss(line);
    std::vector<std::string> args;

    std::string word;
    while (iss >> word) {
        args.push_back(utils::strToUpper(word)); //All uppercase for matching.
    }

    return args;
}

template<typename T0, typename T1>
inline bool isInMapKeys(T0& key, std::unordered_map<T0, T1>& mapToSearch) {
	auto it = mapToSearch.find(key);
	return it != mapToSearch.end();
}


//Generic command function type;
//Takes args, returns something to print to console. Empty string doesn't print anything.
typedef std::function<bool (const std::vector<std::string>&, std::string*)> cmd_t;



//Value changing commands
bool handleConfigValue(const std::vector<std::string>& args, std::string* result) {
	//Sets a value in userConfig, if the key is valid.
	//If only the key is present, return the value stored currently.

	switch (args.size()) {
		case 1u: { //Cannot set/read nothing.
			*result = "CFG requires a function from the config section of userConfig.xml as arg 2, and optional value to set, as arg 3.";
			return false;
		}
		case 2u: { //Read value, if in the map.
			std::string key = args[1];
			if (isInMapKeys(key, userConfig)) {
				*result = "[" + key + "] → [" + userConfig[key] + "]";
			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}
		case 3u: {
			std::string key = args[1];
			std::string newValue = args[2];
			if (isInMapKeys(key, userConfig)) {
				//Add some sort of type checking later. Currently relies on good-faith.
				userConfig[key] = newValue;
			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}
	}
	return true;
}


bool handleKeyValue(const std::vector<std::string>& args, std::string* result) {
	switch (args.size()) {
		case 1u: {
			*result = "KEY requires a function from the keybinds section of userConfig.xml as arg 2, and optional value to set, as arg 3.";
			return false;
		}

		case 2u: { //Inverts current key value.
			std::string key = args[1];
			if (isInMapKeys(key, keyMap)) {
				keyMap[key] = !keyMap[key];
			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}

		case 3u:{ //Sets to specific value.
			std::string key = args[1];
			std::string sValue = args[2];
			if (isInMapKeys(key, keyMap)) {

				bool bValue = (sValue == "TRUE") || (sValue == "T");
				if ((!bValue) && ((sValue != "FALSE") || (sValue != "F"))) {
					*result = "Unknown value for function: [" + key + "] → [" + sValue + "]";
					return false;
				}

				keyMap[key] = bValue;

			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}
	}

	return true;
}



std::unordered_map<std::string, unsigned int> validStageKeys = {
	{"NAME", 0u}, {"FILE_PATH", 1u}, {"SKYBOX_TEXTURE_PATH", 2u},
	{"FOG_COLOUR", 3u}, {"SUN_DIRECTION", 4u}, {"SUN_COLOUR", 5u},
	{"GRAVITY", 6u}, {"KILL_PLANE_Z", 7u}, {"PLAYER_START_POS", 8u},
	{"PLAYER_START_ANGLE", 9u}, {"PLAYER_START_HEALTH", 10u},
	{"PLAYER_START_ENERGY", 11u}
};
bool handleStageValues(const std::vector<std::string>& args, std::string* result) {
	switch (args.size()) {
		case 1u: {
			*result = "SET requires an attrbute from the meta node of the stage xml file as arg 2, and optional value to set, as arg 3.";
			return false;
		}

		case 2u: { //Shows current value.
			std::string key = args[1];
			if (isInMapKeys(key, keyMap)) {
				switch (validStageKeys[key]) { //Assign value in struct from key.
					case 0u:  {*result = "[" + key + "] → [" + stageData.name + "]"; break; }
					case 1u:  {*result = "[" + key + "] → [" + stageData.filePath + "]"; break; }
					case 2u:  {*result = "[" + key + "] → [" + stageData.skyboxTextureName + "]"; break; }
					case 3u:  {*result = "[" + key + "] → [" + std::to_string(stageData.fogColour.x) + ", " + std::to_string(stageData.fogColour.y) + ", " + std::to_string(stageData.fogColour.z) + "]"; break; }
					case 4u:  {*result = "[" + key + "] → [" + std::to_string(stageData.sunDirection.x) + ", " + std::to_string(stageData.sunDirection.y) + ", " + std::to_string(stageData.sunDirection.z) + "]"; break; }
					case 5u:  {*result = "[" + key + "] → [" + std::to_string(stageData.sunColour.x) + ", " + std::to_string(stageData.sunColour.y) + ", " + std::to_string(stageData.sunColour.z) + "]"; break; }
					case 6u:  {*result = "[" + key + "] → [" + std::to_string(stageData.gravity) + "]"; break; }
					case 7u:  {*result = "[" + key + "] → [" + std::to_string(stageData.killPlaneZ) + "]"; break; }
					case 8u:  {*result = "[" + key + "] → [" + std::to_string(stageData.playerStartPoint.x) + ", " + std::to_string(stageData.playerStartPoint.y) + ", " + std::to_string(stageData.playerStartPoint.z) + "]"; break; }
					case 9u:  {*result = "[" + key + "] → [" + std::to_string(stageData.playerStartAngle) + "]"; break; }
					case 10u: {*result = "[" + key + "] → [" + std::to_string(stageData.playerStartHealth) + "]"; break; }
					case 11u: {*result = "[" + key + "] → [" + std::to_string(stageData.playerStartEnergy) + "]"; break; }
				}
			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}

		default: { //Sets to specific value. Has unknown number of additional arguments
			std::string key = args[1];
			if (isInMapKeys(key, validStageKeys)) {
				switch (validStageKeys[key]) { //Assign value in struct from key.
					case 0u: { //Stage name.
						stageData.name = args[2];
						break;
					}
					case 1u: { //Stage filepath [Probably affects nothing.]
						stageData.filePath = args[2];
						break;
					}
					case 2u: { //Skybox texture path [Also probably affects nothing.]
						stageData.skyboxTextureName = args[2];
						break;
					}
					case 3u: { //Fog colour
						if (args.size() < 5u) {
							//Not enough args; requires cmd, key and R/G/B values.
							*result = "Not enough arguments for SET FOG_COLOUR. Requires R/G/B values [0.0-1.0]";
							return false;
						}
						try {
							stageData.fogColour = glm::vec3(
								std::stof(args[2]), std::stof(args[3]), std::stof(args[4])
							);
						} catch (const std::invalid_argument) {
							*result = "Could not convert RGB values to floats.";
							return false;
						}
						break;
					}
					case 4u: { //Sun direction
						if (args.size() < 5u) {
							//Not enough args; requires cmd, key and X/Y/Z values.
							*result = "Not enough arguments for SET SUN_DIRECTION. Requires X/Y/Z values";
							return false;
						}
						try {
							stageData.sunDirection = glm::normalize(glm::vec3(
								std::stof(args[2]), std::stof(args[3]), std::stof(args[4])
							));
						} catch (const std::invalid_argument) {
							*result = "Could not convert XYZ values to floats.";
							return false;
						}
						break;
					}
					case 5u: { //Sun colour
						if (args.size() < 5u) {
							//Not enough args; requires cmd, key and R/G/B values.
							*result = "Not enough arguments for SET SUN_COLOUR. Requires R/G/B values [0.0-1.0]";
							return false;
						}
						try {
							stageData.sunColour = glm::vec3(
								std::stof(args[2]), std::stof(args[3]), std::stof(args[4])
							);
						} catch (const std::invalid_argument) {
							*result = "Could not convert RGB values to floats.";
							return false;
						}
						break;
					}
					case 6u: { //Gravity
						try {
							stageData.gravity = std::stof(args[2]);
						} catch (const std::invalid_argument) {
							*result = "Could not convert value to float.";
							return false;
						}
						break;
					}
					case 7u: { //Kill Plane Z
						try {
							stageData.killPlaneZ = std::stof(args[2]);
						} catch (const std::invalid_argument) {
							*result = "Could not convert value to float.";
							return false;
						}
						break;
					}
					case 8u: { //Player start point
						if (args.size() < 5u) {
							//Not enough args; requires cmd, key and X/Y/Z values.
							*result = "Not enough arguments for SET PLAYER_START_POS. Requires X/Y/Z values";
							return false;
						}
						try {
							stageData.playerStartPoint = glm::vec3(
								std::stof(args[2]), std::stof(args[3]), std::stof(args[4])
							);
						} catch (const std::invalid_argument) {
							*result = "Could not convert XYZ values to floats.";
							return false;
						}
						break;
					}
					case 9u: { //Player start angle
						try {
							stageData.playerStartAngle = std::stof(args[2]);
						} catch (const std::invalid_argument) {
							*result = "Could not convert value to float.";
							return false;
						}
						break;
					}
					case 10u: { //Player start health
						try {
							stageData.playerStartHealth = glm::clamp(std::stoi(args[2]), 0, playerConfig::PLAYER_MAX_HEALTH);
						} catch (const std::invalid_argument) {
							*result = "Could not convert value to int.";
							return false;
						}
						break;
					}
					case 11u: { //Player start energy
						try {
							stageData.playerStartEnergy = glm::clamp(std::stoi(args[2]), 0, playerConfig::PLAYER_MAX_ENERGY);
						} catch (const std::invalid_argument) {
							*result = "Could not convert value to int.";
							return false;
						}
						break;
					}
				}
			} else {
				*result = "Unknown function: [" + key + "]";
				return false;
			}

			break;
		}
	}

	return true;
}


bool handleFlagValues(const std::vector<std::string>& args, std::string* result) {
	switch (args.size()) {
		case 1u: {
			*result = "KEY requires a flag from the flags section of the stage xml file as arg 2, and optional value to set, as arg 3.";
			return false;
		}

		case 2u: { //Shows current flag value.
			std::string key = args[1];
			if (isInMapKeys(key, flagList)) {
				*result = "[" + key + "] → [" + std::to_string(*flagList[key]) + "]";
			} else {
				*result = "Unknown flag: [" + key + "]";
				return false;
			}

			break;
		}

		case 3u:{ //Sets to specific value.
			std::string key = args[1];
			std::string sValue = args[2];
			if (isInMapKeys(key, flagList)) {

				bool bValue = (sValue == "TRUE") || (sValue == "T");
				if ((!bValue) && ((sValue != "FALSE") || (sValue != "F"))) {
					*result = "Unknown value for flag: [" + key + "] → [" + sValue + "]";
					return false;
				}

				*flagList[key] = bValue;

			} else {
				*result = "Unknown flag: [" + key + "]";
				return false;
			}

			break;
		}
	}

	return true;
}



//Random commands
bool handleExit(const std::vector<std::string>& args, std::string* result) {
	keyMap["META_EXIT"] = true; //Causes main-loop to exit.
	return true;
}


bool handleReset(const std::vector<std::string>& args, std::string* result) {
	//Resets the player.
	player.position = stageData.playerStartPoint;
	player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	player.viewAngle = stageData.playerStartAngle * constants::TO_RAD;
	player.state = E_RESPAWN;
	player.touchingFloor = false;
	player.health = playerConfig::PLAYER_MAX_HEALTH;
	player.energy = playerConfig::PLAYER_MAX_ENERGY;
	particles::createParticleRing(player.position, 0.5f, P_HURT, 64);
	return true;
}


bool handleReload(const std::vector<std::string>& args, std::string* result) {
	//Reloads the stage. Does not reload textures.
	stage::reload(true);
	return true;
}







//Resolutions
bool changeRenderResolution(const std::vector<std::string>& args, std::string* result) {
	//Change render resolution to new value.
	switch (args.size()) {
		case 1u: { //Shows current res.
			*result = "Desired: [" + std::to_string(desiredRenderResolution.x) + ", " + std::to_string(desiredRenderResolution.y) + "]   |   Actual: [" + std::to_string(currentRenderResolution.x) + ", " + std::to_string(currentRenderResolution.y) + "]";
			return true;
		}

		case 2u: { //Assume argument is for height; (1080 -> 1920x1080)
			float aspectRatio = currentRenderResolution.x / currentRenderResolution.y;
			std::string newHeightS = args[1];
			int newHeightI;
			try {
				newHeightI = std::stoi(newHeightS);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert vertical resolution to an integer.";
				return false;
			}

			if (newHeightI <= 0) {
				*result = "Cannot have negative or 0 vertical resolution.";
				return false;
			}

			glm::ivec2 newResolution = glm::ivec2(
				static_cast<int>((static_cast<float>(newHeightI) * aspectRatio) + 0.5f), //Rounded to nearest px.
				newHeightI
			);
			desiredRenderResolution = newResolution;
			currentRenderResolution = glm::min(currentWindowResolution, desiredRenderResolution);

			break;
		}

		case 3u: { //Explicit Width/Height values.
			std::string hRes = args[1];
			std::string vRes = args[2];
			glm::ivec2 newResolution;

			try {
				newResolution = glm::ivec2(
					std::stoi(hRes), std::stoi(vRes)
				);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert resolution to integer values.";
				return false;
			}

			if ((newResolution.x <= 0) || (newResolution.y <= 0)) {
				*result = "Cannot have negative or 0 resolution.";
				return false;
			}

			desiredRenderResolution = newResolution;
			currentRenderResolution = glm::min(currentWindowResolution, desiredRenderResolution);

			break;
		}
	}

	//Resize as needed;
	//Queue these changes for the main thread somehow; Cannot call them from console thread.
	GLIndex::screenshotImage2D = graphics::createGLImage2D(currentRenderResolution.x, currentRenderResolution.y);
	GLIndex::wallIntersectSSBO = graphics::createShaderStorageBufferObject(
		7, sizeof(structs::WallIntersect) * currentRenderResolution.x * validWalls
	);
	GLIndex::frameFBO = graphics::createEnvironmentFBO(currentRenderResolution);
	GLIndex::displacementFBO = graphics::createDisplacementsFBO(currentRenderResolution.x, currentRenderResolution.y);
	verticalFOV = 2.0f * atan(tan(utils::configToFloat("VIEW_FOV") * 0.5f * constants::TO_RAD) * (float(currentRenderResolution.y) / float(currentRenderResolution.x)));

	return true;
}



bool changeWindowResolution(const std::vector<std::string>& args, std::string* result) {
	//Change window resolution to new value.
	switch (args.size()) {
		case 1u: { //Shows current res.
			*result = "Actual: [" + std::to_string(currentWindowResolution.x) + ", " + std::to_string(currentWindowResolution.y) + "]";
			return true;
		}
		case 2u: { //Assume argument is for height; (1080 -> 1920x1080)
			float aspectRatio = currentWindowResolution.x / currentWindowResolution.y;
			std::string newHeightS = args[1];
			int newHeightI;
			try {
				newHeightI = std::stoi(newHeightS);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert vertical resolution to an integer.";
				return false;
			}

			if (newHeightI <= 0) {
				*result = "Cannot have negative or 0 vertical resolution.";
				return false;
			}

			glm::ivec2 newResolution = glm::ivec2(
				static_cast<int>((static_cast<float>(newHeightI) * aspectRatio) + 0.5f), //Rounded to nearest px.
				newHeightI
			);
			currentWindowResolution = newResolution;

			break;
		}

		case 3u: { //Explicit Width/Height values.
			std::string hRes = args[1];
			std::string vRes = args[2];
			glm::ivec2 newResolution;

			try {
				newResolution = glm::ivec2(
					std::stoi(hRes), std::stoi(vRes)
				);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert resolution to integer values.";
				return false;
			}

			if ((newResolution.x <= 0) || (newResolution.y <= 0)) {
				*result = "Cannot have negative or 0 resolution.";
				return false;
			}

			currentWindowResolution = newResolution;

			break;
		}
	}

	glfwSetWindowSize(Window, currentWindowResolution.x, currentWindowResolution.y);
	return true;
}



bool changeShadowResolution(const std::vector<std::string>& args, std::string* result) {
	//Change shadows resolution to new value.
	switch (args.size()) {
		case 1u: { //Shows current res.
			*result = "Actual: [" + std::to_string(currentShadowResolution.x) + ", " + std::to_string(currentShadowResolution.y) + "]";
			return true;
		}
		case 2u: { //Assume argument is for height; (1080 -> 1920x1080)
			float aspectRatio = currentShadowResolution.x / currentShadowResolution.y;
			std::string newHeightS = args[1];
			int newHeightI;
			try {
				newHeightI = std::stoi(newHeightS);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert vertical resolution to an integer.";
				return false;
			}

			if (newHeightI <= 0) {
				*result = "Cannot have negative or 0 vertical resolution.";
				return false;
			}

			glm::ivec2 newResolution = glm::ivec2(
				static_cast<int>((static_cast<float>(newHeightI) * aspectRatio) + 0.5f), //Rounded to nearest px.
				newHeightI
			);
			currentShadowResolution = newResolution;

			break;
		}

		case 3u: { //Explicit Width/Height values.
			std::string hRes = args[1];
			std::string vRes = args[2];
			glm::ivec2 newResolution;

			try {
				newResolution = glm::ivec2(
					std::stoi(hRes), std::stoi(vRes)
				);
			} catch (const std::invalid_argument) {
				*result = "Cannot convert resolution to integer values.";
				return false;
			}

			if ((newResolution.x <= 0) || (newResolution.y <= 0)) {
				*result = "Cannot have negative or 0 resolution.";
				return false;
			}

			currentShadowResolution = newResolution;

			break;
		}
	}

	GLIndex::lightingMapsArrayID = graphics::createGLImage2DArray(currentShadowResolution.x, currentShadowResolution.y, validLights + 2);
	return true;
}













namespace console {

	inline std::unordered_map<std::string, cmd_t> commands = {
		{"CFG", handleConfigValue},
		{"KEY", handleKeyValue},
		{"SET", handleStageValues},

		{"QUIT", handleExit},
		{"RESET", handleReset},
		{"RELOAD", handleReload},

		{"RES_RENDER", changeRenderResolution},
		{"RES_WINDOW", changeWindowResolution},
		{"RES_SHADOW", changeShadowResolution}
	};


	void exec(const std::string& line) {
		std::vector<std::string> args = split(line);

		if (args.size() == 0u) {
			return; //Invalid, empty command.
		}

		std::string cmd = args[0];
		if (isInMapKeys(cmd, commands)) {
			std::string result = "";
			bool success;
			{
				std::lock_guard<std::mutex> lock(execCommandMutex);
				success = commands[cmd](args, &result);
			}
			if (!success) {std::cout << "Command failed to execute correctly: " << result << std::endl;}
			else if (!result.empty()) {std::cout << result << std::endl;}
		} else {
			std::cout << "Unknown command: " + cmd << std::endl;
			return; //Unknown command.
		}
	}

}



#endif
