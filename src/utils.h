#ifndef UTILS_H
#define UTILS_H

#include "includes.h"
#include "constants.h"
#include <vector>
#include <stdexcept>
#include <C:/Users/User/Documents/code/.cpp/glm/glm.hpp>

using namespace std;


namespace logicFunctions {
	static void AND(int* A, int* B, int* Q, int* internalState) {*Q = (*A) & (*B);}
	static void OR(int* A, int* B, int* Q, int* internalState) {*Q = (*A) | (*B);}
	static void NOT(int* A, int* B, int* Q, int* internalState) {*Q = ~(*A);}
	static void XOR(int* A, int* B, int* Q, int* internalState) {*Q = (*A) ^ (*B);}

	static void LATCH(int* A, int* B, int* Q, int* internalState) { //Swap between 1 and 0 with A and B.
		if (((*A) & (*B)) > 0) {
			//internalState remains unchanged; both inputs counteract each other's change.
		} else if ((*A) > 0) {
			*internalState = 1;
		} else if ((*B) > 0) {
			*internalState = 0;
		}
		*Q = *internalState;
	}

	static void PULSE(int* A, int* B, int* Q, int* internalState) { //If A is 1, return 1 for a single frame.
		if (((*internalState) < 1) && ((*A) == 1)) {*Q = 1;}
		else {*Q = 0;}
		*internalState = *A;
	}

	static void TOGGLE(int* A, int* B, int* Q, int* internalState) { //Toggles between 1 and 0 if A is 1.
		if ((*A) == 1) {
			*internalState = ((*internalState) < 1) ? 1 : 0;
		}
		*Q = *internalState;
	}

	static void PASSTHROUGH(int* A, int* B, int* Q, int* internalState) {*Q = *A;}
}


//Utility functions
namespace utils {
	static inline void print(std::string str) {
		std::cout << str << std::endl;
	}
	static inline void printVec2(glm::vec2 vector) {
		std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
	}
	static inline void printVec3(glm::vec3 vector) {
		std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl;
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


	static inline bool logicToBool(int A) {return (A > 0);}
	static inline int boolToLogic(bool A) {return (A) ? 1 : 0;}
	static inline bool isVec2NaN(glm::vec2 v) {return (std::isnan(v.x) || std::isnan(v.y));}
	static inline bool isVec3NaN(glm::vec3 v) {return (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z));}



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
					case G_AND: evalGate = logicFunctions::AND; break;
					case G_OR: evalGate = logicFunctions::OR; break;
					case G_NOT: evalGate = logicFunctions::NOT; break;
					case G_XOR: evalGate = logicFunctions::XOR; break;
					case G_LATCH: evalGate = logicFunctions::LATCH; break;
					case G_PULSE: evalGate = logicFunctions::PULSE; break;
					case G_TOGGLE: evalGate = logicFunctions::TOGGLE; break;
					default: evalGate = logicFunctions::PASSTHROUGH; break;
				}
			}

		public:
			GateType gateType;
			int internalState;

			LogicGate() {
				this->gateType = G_INVALID;
				this->evalGate = logicFunctions::PASSTHROUGH;

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
		VisplaneType specialType;
		int* IOPtr;
		float data;
		float internal;

		Visplane()
			: start(0.0f, 0.0f), end(0.0f, 0.0f), height(0.0f), textureID(0), valid(0), specialType(V_INVALID), IOPtr(nullptr), data(0.0f), internal(0.0f) {}

		Visplane(glm::vec2 start, glm::vec2 end, float heightZ, int textureID, VisplaneType specialType=V_NORMAL, int* IOPtr=nullptr, float data=0)
			: start(start), end(end), height(heightZ), 
			  textureID(textureID), valid(1),
			  specialType(specialType),
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
		int textureID;
		int valid;
		WallType specialType;
		int* IOPtr;
		float data;
		float internal;

		Wall()
			: start(0.0f, 0.0f, 0.0f),
			  end(0.0f, 0.0f, 0.0f),
			  textureID(0), valid(0),
			  specialType(W_INVALID),
			  IOPtr(nullptr), data(0.0f),
			  internal(0.0f) {}

		Wall(glm::vec2 start, glm::vec2 end, float lowZ, float topZ, int textureID, WallType specialType=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(glm::vec3(start.x, start.y, lowZ)),
			  end(glm::vec3(end.x, end.y, topZ)), 
			  textureID(textureID), valid(1), 
			  specialType(specialType), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}

		Wall(glm::vec3 start, glm::vec3 end, int textureID, WallType specialType=W_NORMAL, int* IOPtr=nullptr, float data=0.0f)
			: start(start), end(end), 
			  textureID(textureID), valid(1), 
			  specialType(specialType), 
			  IOPtr(IOPtr), data(data), 
			  internal(0.0f) {}
	};

	struct WallGPU {
		alignas(16) glm::vec3 start;
		alignas(16) glm::vec3 end;
		alignas(4) int textureID;
		alignas(4) int valid;
		alignas(8) float _padding[2];

		WallGPU()
			: start(glm::vec3(0.0f, 0.0f, 0.0f)), end(glm::vec3(0.0f, 0.0f, 0.0f)),
			  textureID(0), valid(0), 
			  _padding{0.0f, 0.0f} {}

		WallGPU(Wall *wall)
			: start(wall->start), end(wall->end),
			  textureID(wall->textureID), valid(wall->valid),
			  _padding{0.0f, 0.0f} {}
	};


	struct Sprite {
		glm::vec3 position;
		float width;
		int textureID;
		int valid;
		int collision;
		SpriteType type;

		Sprite() : position(0.0f, 0.0f, 0.0f), width(0.0f), textureID(0), valid(0), type(SPR_INVALID), collision(false) {}

		Sprite(glm::vec3 position, float width, int textureID, SpriteType type=SPR_DECO, int collision=1)
			: position(position), width(width), textureID(textureID), valid(1), type(type), collision(collision) {}
	};

	struct SpriteGPU {
		alignas(16) glm::vec3 position;
		alignas(4) float width;
		alignas(4) int textureID;
		alignas(4) int valid;
		alignas(4) float _padding;

		SpriteGPU() : position(0.0f, 0.0f, 0.0f), width(0.0f), textureID(0), valid(0), _padding(0.0f) {}

		SpriteGPU(Sprite* sprite)
			: position(sprite->position), width(sprite->width),
			  textureID(sprite->textureID), valid(sprite->valid),
			  _padding(0.0f) {}
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





	struct Ray {
		glm::vec2 position, direction, end;

		Ray(glm::vec2 position, glm::vec2 direction, float len=display::MAX_RAY_DIST)
			: position(position), direction(direction), end(position + (direction * len)) {}
	};



	struct Player {
		glm::vec3 position, velocity, cameraPosition;
		float viewAngle, viewRoll, viewPitch, vLook, height;
		bool touchingFloor, sliding;
		Event state;
		int health, energy;
		int jumpsUsed;
		//std::vector<utils::Weapon, constants::MAX_ITEMS_HELD> backpack;

		Player(glm::vec3 position, float angle)
			: position(position), velocity(glm::vec3(0.0f, 0.0f, 0.0f)),
			  cameraPosition(position + glm::vec3(0.0f, 0.0f, playerConfig::PLAYER_COLLISION_HEIGHT_STAND/3.0f)),
			  viewAngle(angle), viewRoll(0.0f), viewPitch(0.0f), vLook(0.0f),
			  height(playerConfig::PLAYER_COLLISION_HEIGHT_STAND), touchingFloor(false),
			  health(playerConfig::PLAYER_MAX_HEALTH), energy(playerConfig::PLAYER_MAX_ENERGY),
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
}

#endif