#ifndef UTILS_H
#define UTILS_H

#include "includes.h"
#include "global.h"
#include "constants.h"
#include <vector>
#include <stdexcept>

using namespace std;



//Utility functions
namespace utils {
	static inline void hideConsole() {
	#ifdef _WIN32
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	#endif
	}

	static inline void showConsole() {
	#ifdef _WIN32
		ShowWindow(GetConsoleWindow(), SW_SHOW);
	#endif
	}

	static inline bool isConsoleVisible() {
	#ifdef _WIN32
		return IsWindowVisible(GetConsoleWindow());
	#elif __linux__
		return true;
	#endif 
	}


	static inline void print(std::string str) {
		if (isConsoleVisible()) {
			std::cout << str << std::endl;
		}
	}
	static inline void printVec2(glm::vec2 vector, std::string name="") {
		if (isConsoleVisible()) {
			if (name.empty()) {
				std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
			} else {
				std::cout << name << " = (" << vector.x << ", " << vector.y << ")" << std::endl;
			}
		}
	}
	static inline void printVec3(glm::vec3 vector, std::string name="") {
		if (isConsoleVisible()) {
			if (name.empty()) {
				std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl;
			} else {
				std::cout << name << " = (" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl;
			}
		}
	}
	static inline void printVec4(glm::vec4 vector, std::string name="") {
		if (isConsoleVisible()) {
			if (name.empty()) {
				std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ")" << std::endl;
			} else {
				std::cout << name << " = (" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ")" << std::endl;
			}
		}
	}
	static inline void printMat4(glm::mat4 matrix, std::string name="") {
		if (isConsoleVisible()) {
			if (name.empty()) {
				std::cout << "[" << std::endl;
			} else {
				std::cout << name << " = [" << std::endl;
			}
			for (size_t x=0; x<4; x++) {
				std::cout << "	";
				for (size_t y=0; y<4; y++) {
					std::cout << matrix[x][y] << ", ";
				}
				std::cout << std::endl;
			}
			std::cout << "]" << std::endl;
		}
	}
	static inline void raise(std::string err) {
		std::cerr << err << std::endl;
		std::string end;
		std::cin >> end;
	}
	static inline void pause() {
		string pause;
		std::cin >> pause;
	}
	static inline void GLErrorcheck(std::string location = "", bool shouldPause = false) {
		GLenum GLError;
		GLError = glGetError();
		if (GLError != GL_NO_ERROR) {
			if (!utils::isConsoleVisible()) {
				utils::showConsole();
			}
			std::cerr << location << " | OpenGL error; " << GLError << std::endl;
			if (shouldPause) {pause();}
		}
	}

	std::string readFile(const std::string& filePath);

	static inline std::string getTimestamp() {
		time_t now = time(nullptr);
		struct tm* timeinfo = localtime(&now);

		std::ostringstream oss;
		oss << std::put_time(timeinfo, "%Y%m%d%H%M%S");

		return oss.str();
	}




	static inline std::string strToLower(const std::string& input) {
		std::string result = input;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){return std::tolower(c);});
		return result;
	}

	static inline std::string strToUpper(const std::string& input) {
		std::string result = input;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){return std::toupper(c);});
		return result;
	}

	static inline bool checkIfInUserConfig(const std::string configName) {
		return userConfig.find(configName) != userConfig.end();
	}
	static inline bool configToBool(const std::string configName) {
		if (checkIfInUserConfig(configName)) {
			std::string configValue = userConfig[configName];
			if ((configValue == "TRUE") || (configValue == "T")) {
				return true;
			} else if ((configValue == "FALSE") || (configValue == "F")) {
				return false;
			} else {
				raise("Unknown config value for " + configName + ": " + configValue);
			}
		} else {
			raise("Unknown config name: " + configName);
		}
		return false;
	}
	static inline int configToIntBool(const std::string configName) {return (configToBool(configName)) ? 1 : 0;}
	static inline int configToInt(const std::string configName) {
		if (checkIfInUserConfig(configName)) {
			std::string valueString = userConfig[configName];
			try {
				return std::stoi(valueString);
			} catch (const std::invalid_argument) {
				raise("Unable to convert " + valueString + " for: " + configName + " to an integer.");
			}
		} else {
			raise("Unknown config name: " + configName);
		}
		return 0;
	}
	static inline float configToFloat(const std::string configName) {
		if (checkIfInUserConfig(configName)) {
			std::string valueString = userConfig[configName];
			try {
				return std::stof(valueString);
			} catch (const std::invalid_argument) {
				raise("Unable to convert " + valueString + " for: " + configName + " to a floating-point value.");
			}
		} else {
			raise("Unknown config name: " + configName);
		}
		return 0.0f;
	}
	static inline bool isPressed(const std::string keyFunction) {
		if (keyMap.find(keyFunction) != keyMap.end()) {
			return keyMap.at(keyFunction);
		} else {
			std::cout << "[" << keyFunction << "] was not bound to a key." << std::endl;
			return false;
		}
	}
	static inline void setPressed(const std::string keyFunction, bool state) {
		if (keyMap.find(keyFunction) != keyMap.end()) {
			keyMap.at(keyFunction) = state;
		}
	}


	static inline bool logicToBool(int A) {return (A > 0);}
	static inline int boolToLogic(bool A) {return (A) ? 1 : 0;}

	static inline bool isVec2NaN(glm::vec2 v) {return (std::isnan(v.x) || std::isnan(v.y));}
	static inline bool isVec3NaN(glm::vec3 v) {return (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z));}

	template<typename T>
	static inline void combineVectors(std::vector<T>* A, std::vector<T>& B) {
		A->insert(A->end(), B.begin(), B.end());
	}

	static inline float getAverage(std::vector<float>& q) {
		float n = 0.0f;
		float sum = 0.0f;
		for (float v : q) {
			sum += v;
			n++;
		}
		return sum / n;
	}


	float determinant(glm::vec2 vecA, glm::vec2 vecB);


	int RNGc(); //Client
	int RNGw(); //World
	void clearRNG(); //Reset both



	bool circleWallIntersect(structs::Wall& line, glm::vec2 circlePosition, float radius, float* distToLine=nullptr);






	static inline void hurtPlayer(int delta) {
		player.health = min(playerConfig::PLAYER_MAX_HEALTH, (player.health) + delta);
		if (delta > 0) {player.state = E_HEAL;}
		else if (player.health > 0) {player.state = E_DEAD;}
		else {player.state = E_HURT;}
	}

	static inline void pickUpItem(ItemFloor itemType) {
		switch (itemType) {
			case IF_HEALTH_SMALL:
				hurtPlayer(playerConfig::HEAL_SMALL);
				break;
			case IF_HEALTH_LARGE:
				hurtPlayer(playerConfig::HEAL_LARGE);
				break;

			case IF_ENERGY_SMALL:
				player.energy = min(playerConfig::PLAYER_MAX_ENERGY, (player.energy) + playerConfig::ENERGY_SMALL);
				player.state = E_ENERGY;
				break;
			case IF_ENERGY_LARGE:
				player.energy = min(playerConfig::PLAYER_MAX_ENERGY, (player.energy) + playerConfig::ENERGY_LARGE);
				player.state = E_ENERGY;
				break;

			case IF_WEAPON:
				player.state = E_NEW_IH;
				//(player.items).push_back() //Add item to inventory.
				break;

			default:
				player.state = E_NONE;
				break;
		}
	}

}

#endif