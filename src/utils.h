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

	static inline bool logicToBool(int A) {return (A > 0);}
	static inline int boolToLogic(bool A) {return (A) ? 1 : 0;}

	static inline bool isVec2NaN(glm::vec2 v) {return (std::isnan(v.x) || std::isnan(v.y));}
	static inline bool isVec3NaN(glm::vec3 v) {return (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z));}


	float determinant(glm::vec2 vecA, glm::vec2 vecB);


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
		bool valid;

		Texture() : dimentions(0.0f, 0.0f), channels(0), data(nullptr), valid(false) {}

		Texture(glm::vec2 dimentions, int channels, unsigned char* data)
			: dimentions(dimentions), channels(channels), data(data), valid(true) {}
	};






	struct Player {
		glm::vec3 position, velocity, cameraPosition;
		float viewAngle, viewRoll, viewPitch, vLook, height;
		bool touchingFloor, sliding;
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
			  state(E_NONE), jumpsUsed(0), sliding(false) {}
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


	static inline int getCentreX(glm::vec3& objPos, Player* player, glm::ivec2 resolution) {
		glm::vec2 direction = glm::normalize(glm::vec2(objPos) - glm::vec2(player->position));
		float theta = atan2(direction.x, direction.y);
		float rayDelta = (theta * constants::TO_DEG) - player->viewAngle;
		if (rayDelta > 180.0f) rayDelta -= 360.0f;
		if (rayDelta < -180.0f) rayDelta += 360.0f;
		float centreX = (resolution.x / 2.0f) * ((rayDelta / rayAngle) + 1.0f);
		return int(round(centreX));
	}


	struct Visplane {
		glm::vec2 start;
		glm::vec2 end;
		float height;
		int textureID;
		VisplaneType specialType;
		int* IOPtr;
		float data;
		float internal;

		Visplane()
			: start(0.0f, 0.0f), end(0.0f, 0.0f), height(0.0f), textureID(0), specialType(V_INVALID), IOPtr(nullptr), data(0.0f), internal(0.0f) {}

		Visplane(glm::vec2 s, glm::vec2 e, float heightZ, int textureID, VisplaneType specialType=V_NORMAL, int* IOPtr=nullptr, float data=0)
			: start(glm::min(s, e)), end(glm::max(s, e)), height(heightZ), 
			  textureID(textureID),
			  specialType(specialType),
			  IOPtr(IOPtr), data(data),
			  internal(0.0f) {}
	};

	struct VisplaneGPU {
		glm::vec2 start;
		glm::vec2 end;
		float height;
		int textureID;
		glm::vec2 _padding;

		VisplaneGPU()
			: start(glm::vec2(0.0f, 0.0f)), end(glm::vec2(0.0f, 0.0f)), height(0.0f),
			  textureID(0), _padding() {}

		VisplaneGPU(Visplane *visplane, Player* player)
			: start(visplane->start), end(visplane->end), height(visplane->height),
			  textureID(visplane->textureID), _padding() {}
	};


	struct Wall {
		glm::vec3 start;
		glm::vec3 end;
		int textureID;
		WallType specialType;
		int* IOPtr;
		float data;
		float internal;

		Wall()
			: start(0.0f, 0.0f, 0.0f),
			  end(0.0f, 0.0f, 0.0f),
			  textureID(0),
			  specialType(W_INVALID),
			  IOPtr(nullptr), data(0.0f),
			  internal(0.0f) {}

		Wall(glm::vec2 start, glm::vec2 end, float lowZ, float topZ, int textureID, WallType specialType=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(glm::vec3(start.x, start.y, lowZ)),
			  end(glm::vec3(end.x, end.y, topZ)), 
			  textureID(textureID),
			  specialType(specialType), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}

		Wall(glm::vec3 start, glm::vec3 end, int textureID, WallType specialType=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(start), end(end), 
			  textureID(textureID),
			  specialType(specialType), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}
	};

	struct WallGPU {
		alignas(16) glm::vec3 start;
		alignas(16) glm::vec3 end;
		alignas(8) glm::vec2 direction;
		alignas(4) int textureID;
		alignas(4) float _padding;

		WallGPU()
			: start(), end(), direction(),
			  textureID(0) {}

		WallGPU(Wall *wall, Player* player)
			: start(wall->start), end(wall->end), direction(glm::normalize(wall->end - wall->start)),
			  textureID(wall->textureID) {}
	};


	struct Displacement {
		std::array<glm::vec3, 3> vertices;
		std::array<glm::vec2, 3> UV;
		glm::vec3 normal;
		int textureID;
		DisplacementType type;
		int* IOPtr;
		float data;
		float internal;

		Displacement() : vertices(), UV(), normal(), textureID(0), type(D_INVALID), data(0.0f), internal(0.0f) {}

		Displacement(
				glm::vec3 vA, glm::vec3 vB, glm::vec3 vC,
				glm::vec2 uvA, glm::vec2 uvB, glm::vec2 uvC,
				int texID, DisplacementType type, int* ptr, float data
			) : vertices{vA, vB, vC}, UV{uvA, uvB, uvC}, textureID(texID),
				type(type), IOPtr(ptr), data(data), internal(0.0f) {
					normal = glm::normalize(glm::cross(
						vB - vA,
						vC - vA
					));
				}

		Displacement(
				std::array<glm::vec3, 3>& verts, std::array<glm::vec2, 3>& texCoords,
				int texID, DisplacementType type, int* ptr, float data
			) : textureID(texID), type(type), IOPtr(ptr),
				data(data), internal(0.0f) {
					for (size_t index=0; index<3; index++) {
						vertices[index] = verts.at(index);
						UV[index] = texCoords.at(index);
					}
					normal = glm::normalize(glm::cross(
						verts[1] - verts[0],
						verts[2] - verts[0]
					));
				}
	};

	struct DisplacementGPU {
		alignas(16) std::array<glm::vec4, 3> vertices;
		alignas(8) std::array<glm::vec2, 3> UV;
		alignas(16) glm::vec4 normal_texID;

		DisplacementGPU() : vertices(), UV(), normal_texID() {}

		DisplacementGPU(Displacement* disp, Player* player) {
				for (size_t index=0; index<3; index++) {
					vertices[index] = glm::vec4(disp->vertices.at(index), 0.0f);
					UV[index] = disp->UV.at(index);
				}
				glm::vec3 pDelta = disp->vertices[0] - player->position;
				glm::vec3 normal = disp->normal * -glm::sign(glm::dot(pDelta, disp->normal));
				normal_texID = glm::vec4(normal, disp->textureID);
			}
	};


	struct Sprite {
		glm::vec3 position;
		float width, height;
		int textureID;
		int collision;
		SpriteType type;

		Sprite() : position(0.0f, 0.0f, 0.0f), width(0.0f), textureID(0), type(SPR_INVALID), collision(false) {}

		Sprite(glm::vec3 position, float width, float height, int textureID, SpriteType type=SPR_DECO, int collision=1)
			: position(position), width(width), height(height), textureID(textureID), type(type), collision(collision) {}
	};

	struct SpriteGPU {
		alignas(16) glm::vec3 position;
		alignas(4) float width;
		alignas(4) float height;
		alignas(4) int textureID;
		alignas(4) int screenCentreX;

		SpriteGPU() : position(0.0f, 0.0f, 0.0f), width(0.0f), height(0.0f), textureID(0), screenCentreX(0) {}

		SpriteGPU(Sprite* sprite, Player* player)
			: position(sprite->position),
			  width(sprite->width), height(sprite->height),
			  textureID(sprite->textureID),
			  screenCentreX(getCentreX(sprite->position, player, currentRenderResolution)) {}
	};


	struct Light {
		glm::vec3 position;
		glm::vec3 colour;
		float intensity;
		int* inputPTR;

		Light() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), inputPTR(nullptr) {}

		Light(glm::vec3 position, glm::vec3 colour, float intensity, int* inputPTR=nullptr)
			: position(position), colour(colour), intensity(intensity), inputPTR(inputPTR) {}
	};

	struct LightGPU {
		alignas(16) glm::vec3 position;
		alignas(16) glm::vec3 colour;
		alignas(4) float intensity;
		alignas(4) bool enabled;
		alignas(4) float _padding;

		LightGPU() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), _padding{0.0f} {}

		LightGPU(Light* light, Player* player)
			: position(light->position), colour(light->colour),
			  intensity(light->intensity),
			  enabled((light->inputPTR == nullptr) ? true : *(light->inputPTR) > 0),
			  _padding{0.0f} {}
	};


	struct TextObject {
		std::string text;
		glm::vec3 position;
		int scale;

		TextObject() : text(""), position(0.0f, 0.0f, 0.0f), scale(0) {}

		TextObject(std::string text, glm::vec3 position, int scale)
			: text(text.substr(0, display::MAX_TEXTOBJECT_CHARACTERS)),
			  position(position),
			  scale(scale) {}
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
		alignas(4) glm::vec2 _padding;

		alignas(16) glm::vec3 position;
		alignas(4) int screenCentreX;

		TextObjectGPU() : text(), position(0.0f, 0.0f, 0.0f), scale(0), screenCentreX(0), _padding() {}

		TextObjectGPU(
			TextObject* textObject, Player* player,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* symbolNames
		)	: length(textObject->text.length()),
			  position(textObject->position),
			  scale(textObject->scale),
			  screenCentreX(getCentreX(textObject->position, player, display::UI_RESOLUTION)),
			  _padding()
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
}

#endif