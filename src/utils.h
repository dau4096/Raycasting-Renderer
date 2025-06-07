#ifndef UTILS_H
#define UTILS_H

#include "includes.h"
#include "global.h"
#include "constants.h"
#include <vector>
#include <stdexcept>
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

using namespace std;


namespace logicFunctions {
	static void LGF_AND(int* A, int* B, int* Q, int* internalState) {*Q = (*A) & (*B);}
	static void LGF_OR(int* A, int* B, int* Q, int* internalState) {*Q = (*A) | (*B);}
	static void LGF_NOT(int* A, int* B, int* Q, int* internalState) {*Q = ~(*A);}
	static void LGF_XOR(int* A, int* B, int* Q, int* internalState) {*Q = (*A) ^ (*B);}

	static void LGF_LATCH(int* A, int* B, int* Q, int* internalState) { //Swap between 1 and 0 with A and B.
		if (((*A) & (*B)) > 0) {
			//internalState remains unchanged; both inputs counteract each other's change.
		} else if ((*A) > 0) {
			*internalState = 1;
		} else if ((*B) > 0) {
			*internalState = 0;
		}
		*Q = *internalState;
	}

	static void LGF_PULSE(int* A, int* B, int* Q, int* internalState) { //If A is 1, return 1 for a single frame.
		if (((*internalState) < 1) && ((*A) == 1)) {*Q = 1;}
		else {*Q = 0;}
		*internalState = *A;
	}

	static void LGF_TOGGLE(int* A, int* B, int* Q, int* internalState) { //Toggles between 1 and 0 if A is 1.
		if ((*A) == 1) {
			*internalState = ((*internalState) < 1) ? 1 : 0;
		}
		*Q = *internalState;
	}

	static void LGF_PASSTHROUGH(int* A, int* B, int* Q, int* internalState) {*Q = *A;}
}


//Utility functions
namespace utils {

	static inline void hideConsole() {
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
	static inline void showConsole() {
		ShowWindow(GetConsoleWindow(), SW_SHOW);
	}
	static inline bool isConsoleVisible() {
		return IsWindowVisible(GetConsoleWindow()) != FALSE;
	}


	static inline void print(std::string str) {
		if (isConsoleVisible()) {
			std::cout << str << std::endl;
		}
	}
	static inline void printVec2(glm::vec2 vector) {
		if (isConsoleVisible()) {
			std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
		}
	}
	static inline void printVec3(glm::vec3 vector) {
		if (isConsoleVisible()) {
			std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl;
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

	static inline bool logicToBool(int A) {return (A > 0);}
	static inline int boolToLogic(bool A) {return (A) ? 1 : 0;}

	static inline bool isVec2NaN(glm::vec2 v) {return (std::isnan(v.x) || std::isnan(v.y));}
	static inline bool isVec3NaN(glm::vec3 v) {return (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z));}


	float determinant(glm::vec2 vecA, glm::vec2 vecB);
	float angleClamp(float value); //Degrees


	int RNGc(); //Client
	int RNGw(); //World
	void clearRNG(); //Reset both
	


	class LogicGate {
		private:
			std::function<void(int*, int*, int*, int*)> evalGate;
			int* inputA;
			int* inputB;
			int* output;

			void _assignEvalFunction() {
				switch (this->gateType) {
					case G_AND: evalGate = logicFunctions::LGF_AND; break;
					case G_OR: evalGate = logicFunctions::LGF_OR; break;
					case G_NOT: evalGate = logicFunctions::LGF_NOT; break;
					case G_XOR: evalGate = logicFunctions::LGF_XOR; break;
					case G_LATCH: evalGate = logicFunctions::LGF_LATCH; break;
					case G_PULSE: evalGate = logicFunctions::LGF_PULSE; break;
					case G_TOGGLE: evalGate = logicFunctions::LGF_TOGGLE; break;
					default: evalGate = logicFunctions::LGF_PASSTHROUGH; break;
				}
			}

		public:
			GateType gateType;
			int internalState;

			LogicGate() {
				this->gateType = G_INVALID;
				this->evalGate = logicFunctions::LGF_PASSTHROUGH;

				this->inputA = nullptr;
				this->inputB = nullptr;
				this->output = nullptr;

				this->internalState = 0;
			}

			LogicGate(GateType gateType, int* output, int* inputA, int* inputB=nullptr) {
				//Has optional inputB.
				this->gateType = gateType;
				_assignEvalFunction();


				this->inputA = inputA;
				this->inputB = inputB;
				this->output = output;

				this->internalState = 0;
			}

			void evaluateState() {
				if (evalGate) {
					evalGate(this->inputA, this->inputB, this->output, &(this->internalState));
				}
			}
	};


	struct Texture {
		glm::vec2 dimentions;
		int channels;
		unsigned char* data;
		int valid;

		Texture() : dimentions(0.0f, 0.0f), channels(0), data(nullptr), valid(0) {}

		Texture(glm::vec2 dimentions, int channels, unsigned char* data)
			: dimentions(dimentions), channels(channels), data(data), valid(1) {}
	};


	struct Visplane {
		glm::vec2 start;
		glm::vec2 end;
		float height;
		int textureID;
		int valid;
		VisplaneType type;
		int* IOPtr;
		float data;
		float internal;

		Visplane()
			: start(0.0f, 0.0f), end(0.0f, 0.0f), height(0.0f), textureID(0), valid(0), type(V_INVALID), IOPtr(nullptr), data(0.0f), internal(0.0f) {}

		Visplane(glm::vec2 start, glm::vec2 end, float heightZ, int textureID, VisplaneType type=V_NORMAL, int* IOPtr=nullptr, float data=0)
			: start(start), end(end), height(heightZ), 
			  textureID(textureID), valid(1),
			  type(type),
			  IOPtr(IOPtr), data(data),
			  internal(0.0f) {}
	};

	struct VisplaneGPU {
		glm::vec2 start;
		glm::vec2 end;
		float height;
		int textureID;
		int valid;
		float _padding;

		VisplaneGPU()
			: start(glm::vec2(0.0f, 0.0f)), end(glm::vec2(0.0f, 0.0f)), height(0.0f),
			  textureID(0), valid(0),
			  _padding(0.0f) {}

		VisplaneGPU(Visplane *visplane)
			: start(visplane->start), end(visplane->end), height(visplane->height),
			  textureID(visplane->textureID), valid(visplane->valid),
			  _padding(0.0f) {}
	};


	struct Wall {
		glm::vec3 start;
		glm::vec3 end;
		glm::vec2 direction;
		int textureID;
		int valid;
		WallType type;
		int* IOPtr;
		float data;
		float internal;

		Wall()
			: start(),
			  end(),
			  direction(),
			  textureID(), valid(0),
			  type(W_INVALID),
			  IOPtr(nullptr), data(),
			  internal() {}

		Wall(glm::vec2 start, glm::vec2 end, float lowZ, float topZ, int textureID, WallType type=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(glm::vec3(start.x, start.y, lowZ)),
			  end(glm::vec3(end.x, end.y, topZ)),
			  direction(glm::normalize(end - start)),
			  textureID(textureID), valid(1), 
			  type(type), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}

		Wall(glm::vec3 start, glm::vec3 end, int textureID, WallType type=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(start), end(end), 
			  direction(glm::normalize(end - start)),
			  textureID(textureID), valid(1), 
			  type(type), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}
	};

	struct alignas(16) WallGPU {
		glm::vec3 start;     float _pad0 = 0.0f;   // 16 bytes
		glm::vec3 end;       float _pad1 = 0.0f;   // 16 bytes
		glm::vec2 direction; glm::vec2 _pad2 = {}; // 16 bytes

		int textureID = 0;
		int type = 0;
		float extra = 0.0f;
		int valid = 0;                             // 16 bytes

		glm::vec2 _padding = {}; glm::vec2 _pad3 = {}; // 16 bytes

		WallGPU() = default;

		WallGPU(Wall* wall)
			: start(wall->start), _pad0(0.0f),
			  end(wall->end), _pad1(0.0f),
			  direction(wall->direction), _pad2(),
			  textureID(wall->textureID),
			  type(static_cast<int>(wall->type)),
			  extra(wall->data),
			  valid(wall->valid),
			  _padding(), _pad3() {}
	};


	struct Sprite {
		glm::vec3 position;
		float width, height;
		int textureID;
		int valid;
		int collision;
		SpriteType type;

		Sprite() : position(0.0f, 0.0f, 0.0f), width(0.0f), textureID(0), valid(0), type(SPR_INVALID), collision(false) {}

		Sprite(glm::vec3 position, float width, float height, int textureID, SpriteType type=SPR_DECO, int collision=1)
			: position(position), width(width), height(height), textureID(textureID), valid(1), type(type), collision(collision) {}
	};

	struct SpriteGPU {
		alignas(16) glm::vec3 position;
		alignas(4) float width;
		alignas(4) float height;
		alignas(4) int textureID;
		alignas(4) int valid;

		SpriteGPU() : position(0.0f, 0.0f, 0.0f), width(0.0f), height(0.0f), textureID(0), valid(0) {}

		SpriteGPU(Sprite* sprite)
			: position(sprite->position),
			  width(sprite->width), height(sprite->height),
			  textureID(sprite->textureID),
			  valid(sprite->valid) {}
	};


	struct Light {
		glm::vec3 position;
		glm::vec3 colour;
		float intensity;
		int valid;
		int* inputPTR;

		Light() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), valid(0), inputPTR(nullptr) {}

		Light(glm::vec3 position, glm::vec3 colour, float intensity, int* inputPTR=nullptr)
			: position(position), colour(colour), intensity(intensity), valid(1), inputPTR(inputPTR) {}
	};

	struct LightGPU {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 colour;
		alignas(4) float intensity;
		alignas(4) int valid;
		alignas(4) float _padding;

		LightGPU() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), valid(0), _padding{0.0f} {}

		LightGPU(Light* light)
			: position(light->position), colour(light->colour),
			  intensity(light->intensity),
			  valid((light->valid) & ((light->inputPTR == nullptr) ? 1 : *(light->inputPTR))),
			  _padding{0.0f} {}
	};


	struct TextObject {
		std::string text;
		glm::vec3 position;
		int scale;
		int valid;

		TextObject() : text(""), position(0.0f, 0.0f, 0.0f), scale(0), valid(0) {}

		TextObject(std::string text, glm::vec3 position, int scale)
			: text(text.substr(0, display::MAX_TEXTOBJECT_CHARACTERS)),
			  position(position),
			  scale(scale),
			  valid(1) {}
	};

	const std::unordered_map<std::string, int> chMap = {
		{"|", -3}, {" ", -2},
		{".", 10}, {"-", 11},
		{"!", 12}, {"?", 13},
		{",", 14}, {"'", 15},
		{"/", 16}, {":", 17},
		{";", 18}, {"&", 19},
		{"[", 20}, {"]", 21},
		{"(", 20}, {")", 21},
		{"^", 22}
	};

	static std::array<int, display::MAX_TEXTOBJECT_CHARACTERS> convertTextToIdxArray(
		const std::string& input,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* symbolNames
	) {
		std::array<int, display::MAX_TEXTOBJECT_CHARACTERS> result;
		result.fill(-1);

		std::string inputUpper = strToUpper(input);

		for (size_t idx=0; (idx<input.size() && idx<display::MAX_TEXTOBJECT_CHARACTERS); ++idx) {
			std::string ch(1, inputUpper[idx]);
			int res;

			auto chMapIt = chMap.find(ch);
			if (chMapIt != chMap.end()) {
				res = chMapIt->second;
			} else {
				auto symNamesIt = std::find(symbolNames->begin(), symbolNames->end(), "symbol_" + ch);
				if (symNamesIt != symbolNames->end()) {
					res = static_cast<int>(std::distance(symbolNames->begin(), symNamesIt));
				} else {
					//Unknown char; show unknown char
					res = 23;
				}
			}
			result[idx] = res;
		}

		return result;
	}

	struct TextObjectGPU {
		alignas(16) std::array<glm::ivec4, display::MAX_TEXTOBJECT_CHARACTERS / 4> text;
		
		alignas(4) int length;
		alignas(4) int scale;
		alignas(4) int valid;
		alignas(4) int _paddingA;

		alignas(16) glm::vec3 position;
		alignas(4) float _paddingB;

		TextObjectGPU() : text(), position(0.0f, 0.0f, 0.0f), scale(0), valid(0), _paddingA(0), _paddingB(0.0f) {}

		TextObjectGPU(
			TextObject* textObject,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* symbolNames
		)	: length(textObject->text.length()),
			  position(textObject->position),
			  scale(textObject->scale),
			  valid(textObject->valid),
			  _paddingA(0), _paddingB(0.0f)
		{
			std::array<int, display::MAX_TEXTOBJECT_CHARACTERS> flat = convertTextToIdxArray(textObject->text, symbolNames);
			for (size_t i = 0; i < display::MAX_TEXTOBJECT_CHARACTERS / 4; ++i) {
				text[i] = glm::ivec4(flat[i * 4 + 0], flat[i * 4 + 1], flat[i * 4 + 2], flat[i * 4 + 3]);
			}
		}
	};





	struct Ray {
		glm::vec2 position, direction, end;

		Ray(glm::vec2 position, glm::vec2 direction, float len=configToFloat("VIEW_MAX_RAY_DIST"))
			: position(position), direction(direction), end(position + (direction * len)) {}
	};



	struct Player {
		glm::vec3 position, velocity, cameraPosition;
		float viewAngle, viewRoll, viewPitch, vLook, height;
		bool touchingFloor, sliding, usedPortal, touchedPortal;
		Event state;
		int health, energy;
		unsigned int jumpsUsed;
		//std::vector<utils::Weapon, constants::MAX_ITEMS_HELD> backpack;

		Player()
			: position(stageData.playerStartPoint), velocity(glm::vec3(0.0f, 0.0f, 0.0f)),
			  cameraPosition(stageData.playerStartPoint + glm::vec3(0.0f, 0.0f, playerConfig::PLAYER_COLLISION_HEIGHT_STAND/3.0f)),
			  viewAngle(stageData.playerStartAngle), viewRoll(0.0f), viewPitch(0.0f), vLook(0.0f),
			  height(playerConfig::PLAYER_COLLISION_HEIGHT_STAND), touchingFloor(false),
			  health(stageData.playerStartHealth), energy(stageData.playerStartEnergy),
			  state(E_NONE), jumpsUsed(0), sliding(false), usedPortal(false), touchedPortal(false) {}
	};


	static inline void hurtPlayer(Player *player, int delta) {
		player->health = min(playerConfig::PLAYER_MAX_HEALTH, (player->health) + delta);
		if (delta > 0) {player->state = E_HEAL;}
		else if (player->health > 0) {player->state = E_DEAD;}
		else {player->state = E_HURT;}
	}

	static inline void pickUpItem(Player *player, ItemFloor itemType) {
		switch (itemType) {
			case IF_HEALTH_SMALL:
				hurtPlayer(player, playerConfig::HEAL_SMALL);
				break;
			case IF_HEALTH_LARGE:
				hurtPlayer(player, playerConfig::HEAL_LARGE);
				break;

			case IF_ENERGY_SMALL:
				player->energy = min(playerConfig::PLAYER_MAX_ENERGY, (player->energy) + playerConfig::ENERGY_SMALL);
				player->state = E_ENERGY;
				break;
			case IF_ENERGY_LARGE:
				player->energy = min(playerConfig::PLAYER_MAX_ENERGY, (player->energy) + playerConfig::ENERGY_LARGE);
				player->state = E_ENERGY;
				break;

			case IF_WEAPON:
				player->state = E_NEW_IH;
				//(player->items).push_back() //Add item to inventory.
				break;

			default:
				player->state = E_NONE;
				break;
		}
	}
}

#endif