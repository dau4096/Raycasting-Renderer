#pragma once
#include "includes.h"
#include "constants.h"
using namespace std;


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
	{"VIEW_MIPMAPPING", ""},
	{"VIEW_TEXTURE_QUALITY", ""},
	{"VIEW_ANTIALIAS_LEVEL", ""},
	{"VIEW_LUMINANCE_QUANTISATION", ""},
	{"VIEW_MAX_FREQ", ""},
	{"VIEW_INTERFACE_IN_SCREENSHOT", ""},
	{"VIEW_VSYNC", ""},
	{"VIEW_SHADOW_QUALITY", ""},
	{"VIEW_ALLOW_TRANSPARENCY", ""},
	{"VIEW_ALLOW_TRANSPARENT_SHADOWS", ""},
	{"VIEW_WIGGLY_TEXTOBJECTS", ""},

	{"META_DEBUG_MODE", ""},
	{"META_SHOW_TICKRATE_UI", ""},
	{"META_SHOW_FRAMERATE_UI", ""},
	{"META_SHOW_FRAMERATE_CONSOLE", ""},
	{"META_SHOW_DT_CONSOLE", ""},
	{"META_SHOW_CONSOLE", ""},
	{"META_STAGE_NAME", ""},
	{"META_SHOW_DATA", ""},
	{"META_DYNAMIC_UPD", ""},
	{"META_DYNAMIC_UPD_ALLOW_NEW_TEXTURES", ""},

	{"PHYS_NO_COLLIDE", ""},
	{"PHYS_FLY", ""},
};



inline std::array<bool, constants::MAX_FLAGS> flags;
inline std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS> textureNames;



//All stage-specific data.
struct StageData {
	std::string name;
	std::string filePath;

	std::string skyboxTextureName;
	glm::vec3 fogColour;

	glm::vec3 sunDirection, sunColour;
	
	float gravity;
	float killPlaneZ;

	glm::vec3 playerStartPoint;
	float playerStartAngle;
	float playerStartHealth, playerStartEnergy;


	StageData()
		: name("<NONE>"), filePath(""),
		  skyboxTextureName("fallback-skybox"), fogColour(0.4157f, 0.6039f, 0.7098f),
		  sunDirection(0.0f, 0.0f, 1.0f), sunColour(1.0f, 1.0f, 1.0f),
		  gravity(0.486), killPlaneZ(-64.0f),
		  playerStartPoint(0.0f, 0.0f, 0.0f), playerStartAngle(0.0f),
		  playerStartHealth(1.0f), playerStartEnergy(1.0f) {}
};
inline StageData stageData;




//Texture Queue;
struct TextureLoadTask {
	GLuint textureArrayID;
	std::string textureName, subFolder;
	int layer;
	bool hasMipMap;

	TextureLoadTask() : textureArrayID(), textureName(), subFolder(), layer(), hasMipMap() {}

	TextureLoadTask(GLuint textureArrayID, int layer, std::string textureName, std::string subFolder="textures-env", bool hasMipMap=true)
	 : textureArrayID(textureArrayID), layer(layer), textureName(textureName), subFolder(subFolder), hasMipMap(hasMipMap) {}
};
inline std::queue<TextureLoadTask> textureLoadQueue;


//Numbers of valid types.
inline size_t validVisplanes = 0;
inline size_t validWalls = 0;
inline size_t validDisplacements = 0;
inline size_t numVisibleVisplanes = 0;
inline size_t numVisibleWalls = 0;
inline size_t numVisibleDisplacements = 0;
inline size_t validSprites = 0;
inline size_t validLights = 0;
inline size_t validTextObjects = 0;
inline size_t validGates = 0;
inline size_t currentTextureIndex = 0;



inline glm::ivec2 currentWindowResolution;
inline glm::ivec2 desiredRenderResolution;
inline glm::ivec2 currentRenderResolution;
inline glm::ivec2 currentShadowResolution;


//Other
inline float framerate;
inline float tickrate;
inline float zoomEffect;
inline float rayAngle;
inline float verticalFOV;
inline size_t frameNumber;
inline size_t tickNumber;

inline float avgframerate, avgtickrate;
inline bool shouldShowFPS, shouldShowTPS;
inline std::vector<float> rollingFPS;
inline std::vector<float> rollingTPS;

inline bool headLampEnabled;
inline bool interactKey;
inline bool prevInteract;
inline bool shouldTakeScreenshot;
inline int lightFlickerRNG;
inline glm::vec4 screenTint;
inline bool isInvertEffect;


//Dataset used by all walls, visplanes etc to sync internal values between physicsDataset and graphicsDataset
inline std::deque<std::pair<float, float>> internalsData;



namespace GLIndex {
//Displacements
inline GLuint dispVAO, dispVBO, dispEBO;
inline GLuint displacementFBO, displacementFBOColour, displacementFBOPosition, displacementFBONormals;

//Assorted
inline GLuint uiVAO, uiVBO, uiEBO;
inline GLuint genericVAO;
inline GLuint renderedFrameID, interfaceID, positionMapID, normalMapID, lightingMapsArrayID;

//Shaders
inline GLuint raycastShader, envShader, displacementShader3D, displacementShader2D;
inline GLuint spriteShader, lightingShader, uiShader, displayShader; 

//Textures
inline GLuint textureArrayEnvironment, skyboxTextureID, textureArrayUI, textureArrayNumeric;

//Storage Buffers and similar.
inline GLuint wallIntersectSSBO, allVisplanesSSBO, allWallsSSBO, spriteSSBO, lightSSBO;
inline GLuint displacementSSBO, visibleVisplaneIndicesSSBO, visibleWallIndicesSSBO;

}





namespace structs {


struct Player {
	glm::vec3 position, prevPosition, velocity, cameraPosition, interpPosition;
	float viewAngle, viewRoll, viewPitch, vLook, height;
	bool touchingFloor, sliding, onConveyor;
	Event state, previousState;
	int health, energy;
	unsigned int jumpsUsed;
	//std::vector<utils::Weapon, constants::MAX_ITEMS_HELD> backpack;

	Player()
		: position(stageData.playerStartPoint), prevPosition(stageData.playerStartPoint), velocity(glm::vec3(0.0f, 0.0f, 0.0f)),
		  cameraPosition(stageData.playerStartPoint + glm::vec3(0.0f, 0.0f, playerConfig::PLAYER_COLLISION_HEIGHT_STAND/3.0f)),
		  interpPosition(stageData.playerStartPoint),
		  viewAngle(stageData.playerStartAngle * constants::TO_RAD), viewRoll(0.0f), viewPitch(0.0f), vLook(0.0f),
		  height(playerConfig::PLAYER_COLLISION_HEIGHT_STAND), touchingFloor(false),
		  health(stageData.playerStartHealth), energy(stageData.playerStartEnergy),
		  state(E_NONE), previousState(E_NONE), jumpsUsed(0), sliding(false) {}
};


static inline int getCentreX(glm::vec3& objPos, Player player, glm::ivec2 resolution) {
	glm::vec2 direction = glm::normalize(glm::vec2(objPos) - glm::vec2(player.interpPosition));
	float theta = atan2(direction.x, direction.y);
	float angleDelta = theta - player.viewAngle;
	if (angleDelta > constants::PI) {angleDelta -= constants::PI2;}
	if (angleDelta < -constants::PI) {angleDelta += constants::PI2;}
	float centreX = (resolution.x / 2.0f) * ((angleDelta * zoomEffect / rayAngle) + 1.0f);
	return int(round(centreX));
}



static inline GLuint combineTextureData1(
	GLint textureID, bool swapUVXY
) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	- animationFlags; (4 bit flags) [0 - 15]
		1111 0000 0000 0000 0000 0000 0000 0000
	- textureID; (12 bit uint) [0 - 65535]
		0000 1111 1111 1111 0000 0000 0000 0000
	*/
	return (
		((GLuint(swapUVXY) & 0x1u) << 31) |
		((GLuint(textureID) & 0xFFFu) << 16)
	);
}


static inline GLuint combineTextureData2(
	bool isWorldSpaceX, bool isWorldSpaceY,
	glm::vec2 textureScale, glm::vec2 textureOffset
) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	isWorldspace.x; (bool) [0 / 1]
		1000 0000 0000 0000 0000 0000 0000 0000
	- isWorldspace.y; (bool) [0 / 1]
		0100 0000 0000 0000 0000 0000 0000 0000
	- textureScale.x; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0011 1111 1100 0000 0000 0000 0000 0000
	- textureScale.y; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0000 0000 0011 1111 1100 0000 0000 0000
	- textureOffset.x; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0011 1111 1000 0000
	- textureOffset.y; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0000 0000 0111 1111
	*/
	return (
		((GLuint(isWorldSpaceX) & 0x1u) << 31) |
		((GLuint(isWorldSpaceY) & 0x1u) << 30) |
		((GLuint(textureScale.x * 16.0f) & 0xFFu) << 22) |
		((GLuint(textureScale.y * 16.0f) & 0xFFu) << 14) |
		((GLuint(textureOffset.x * 128.0f) & 0x7Fu) << 7) |
		((GLuint(textureOffset.y * 128.0f) & 0x7Fu) << 0)
	);
}

static void ensureACW(glm::vec2 vertices[8], size_t numVertices) {
	//Ensures the winding order is always Anti-Clockwise.
	float signedArea = 0.0f;
	for (int i = 0; i < numVertices; ++i) {
		glm::vec2 a = vertices[i];
		glm::vec2 b = vertices[(i + 1) % numVertices];
		signedArea += (b.x - a.x) * (b.y + a.y);
	}

	if (signedArea > 0.0f) {
		//The winding order was clockwise, reverse order.
		for (int i = 0; i < numVertices / 2; ++i) {
			std::swap(vertices[i], vertices[numVertices - 1 - i]);
		}
	}
}

struct Visplane {
	glm::vec2 vertices[8];
	glm::vec2 originalVertices[8];
	size_t numVertices;
	float height, originalHeight;
	GLuint textureData1;
	GLuint textureData2;
	VisplaneType type;
	bool* IOPtr;
	float data;
	std::pair<float, float>* internal;

	Visplane()
		: vertices(), originalVertices(), numVertices(0), height(0.0f), originalHeight(0.0f),
		  textureData1(0), type(V_INVALID), IOPtr(nullptr), data(0.0f) {
			internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
			internal = &(internalsData.at(internalsData.size()-1));
		}

	Visplane(
			std::vector<glm::vec2> verts, float heightZ, GLint textureID,
			VisplaneType type=V_NORMAL, bool* IOPtr=nullptr, float data=0,
			bool isWorldSpaceX=true, bool isWorldSpaceY=true, bool swapUVXY=false,
			glm::vec2 textureScale=glm::vec2(1.0f, 1.0f), glm::vec2 textureOffset=glm::vec2(0.0f, 0.0f),
			float exitDirection=constants::INF
		) : height(heightZ), originalHeight(heightZ), 
			textureData1(textureData1),
			type(type),	IOPtr(IOPtr), data(data) {
				textureData1 = combineTextureData1(
					textureID, swapUVXY
				);
				textureData2 = combineTextureData2(
					isWorldSpaceX, isWorldSpaceY,
					textureScale, textureOffset
				);

				if ((type != V_NORMAL) && (type != V_INVALID)) {
					internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
					internal = &(internalsData.at(internalsData.size()-1));
				} else {
					internal = nullptr;
				}


				if (type == V_TELEPORT) {
					internal->second = exitDirection * constants::TO_RAD;
				}


				numVertices = verts.size();
				size_t index = 0;
				for (glm::vec2 v : verts) {
					vertices[index] = v;
					index++;
				}
				glm::vec2 empty = glm::vec2(0.0f, 0.0f);
				for (size_t newIdx=index; newIdx<8; newIdx++) {
					vertices[newIdx] = empty;
				}
				ensureACW(vertices, numVertices);
				std::copy(std::begin(vertices), std::end(vertices), std::begin(originalVertices));
			}

	Visplane(
			glm::vec2 start, glm::vec2 end, float heightZ, GLint textureID,
			VisplaneType type=V_NORMAL, bool* IOPtr=nullptr, float data=0,
			bool isWorldSpaceX=true, bool isWorldSpaceY=true, bool swapUVXY=false,
			glm::vec2 textureScale=glm::vec2(1.0f, 1.0f), glm::vec2 textureOffset=glm::vec2(0.0f, 0.0f),
			float exitDirection=constants::INF
		) : height(heightZ), originalHeight(heightZ), 
			type(type),	IOPtr(IOPtr), data(data) {
				textureData1 = combineTextureData1(
					textureID, swapUVXY
				);
				textureData2 = combineTextureData2(
					isWorldSpaceX, isWorldSpaceY,
					textureScale, textureOffset
				);

				if ((type != V_NORMAL) && (type != V_INVALID)) {
					internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
					internal = &(internalsData.at(internalsData.size()-1));
				} else {
					internal = nullptr;
				}


				if (type == V_TELEPORT) {
					internal->second = exitDirection * constants::TO_RAD;
				}


				numVertices = 4;
				vertices[0] = start;
				vertices[1] = glm::vec2(start.x, end.y);
				vertices[2] = end;
				vertices[3] = glm::vec2(end.x, start.y);

				glm::vec2 empty = glm::vec2(0.0f, 0.0f);
				vertices[4] = empty; vertices[5] = empty; vertices[6] = empty; vertices[7] = empty;

				ensureACW(vertices, numVertices);
				std::copy(std::begin(vertices), std::end(vertices), std::begin(originalVertices));
			}
};

struct VisplaneGPU {
	glm::vec4 vertices[4];
	GLuint numVertices;
	float height;
	GLuint textureData1;
	GLuint textureData2;
	glm::vec4 boundingBox;

	VisplaneGPU()
		: vertices(), height(0.0f), numVertices(0), textureData1(0), textureData2(), boundingBox() {}

	VisplaneGPU(Visplane *visplane, Player player)
		: numVertices(visplane->numVertices), height(visplane->height),
		  textureData1(visplane->textureData1), textureData2(visplane->textureData2) {
			for (int i=0; i<4; i++) {
				vertices[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
			}

			boundingBox = glm::vec4(
				 constants::INF,  constants::INF,
				-constants::INF, -constants::INF
			);
			for (size_t i=0; i<visplane->numVertices; i += 2) {
				glm::vec2 a = visplane->vertices[i];
				glm::vec2 b = (i+1 < visplane->numVertices) ? visplane->vertices[i+1] : glm::vec2(0.0f, 0.0f);
				vertices[i/2] = glm::vec4(a, b);

				boundingBox.x = min(min(a.x, b.x), boundingBox.x);
				boundingBox.y = min(min(a.y, b.y), boundingBox.y);
				boundingBox.z = max(max(a.x, b.x), boundingBox.z);
				boundingBox.w = max(max(a.y, b.y), boundingBox.w);
			}
		}
};


struct Wall {
	glm::vec3 start, originalStart;
	glm::vec3 end, originalEnd;
	std::pair<GLuint, GLuint> textureData1s;
	GLuint textureData2;
	WallType type;
	bool* IOPtr;
	float data;
	std::pair<float, float>* internal;

	Wall()
		: start(0.0f, 0.0f, 0.0f), originalStart(0.0f, 0.0f, 0.0f),
		  end(0.0f, 0.0f, 0.0f), originalEnd(0.0f, 0.0f, 0.0f),
		  textureData1s(),
		  type(W_INVALID),
		  IOPtr(nullptr), data(0.0f) {
			if ((type != W_NORMAL) && (type != W_INVALID)) {
				internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
				internal = &(internalsData.at(internalsData.size()-1));
			} else {
				internal = nullptr;
			}
		  }

	Wall(
			glm::vec2 start, glm::vec2 end, float lowZ, float topZ,
			GLint textureID0,
			WallType type=W_NORMAL, bool* IOPtr=nullptr, float data=0.0f,
			GLint textureID1=-1,
			bool isWorldSpaceX=true, bool isWorldSpaceY=true, bool swapUVXY1=false, bool swapUVXY2=false,
			glm::vec2 textureScale=glm::vec2(1.0f, 1.0f), glm::vec2 textureOffset=glm::vec2(0.0f, 0.0f)
		) : start(glm::vec3(start.x, start.y, lowZ)), originalStart(glm::vec3(start.x, start.y, lowZ)),
			end(glm::vec3(end.x, end.y, topZ)), originalEnd(glm::vec3(end.x, end.y, topZ)),
			type(type), 
			IOPtr(IOPtr), data(data) {
				textureData1s.first = combineTextureData1(textureID0, swapUVXY1);
				if (textureID1 < 0) {
					textureData1s.second = combineTextureData1(textureID0, swapUVXY2);
				} else {
					textureData1s.second = combineTextureData1(textureID1, swapUVXY2);
				}

				textureData2 = combineTextureData2(
					isWorldSpaceX, isWorldSpaceY,
					textureScale, textureOffset
				);

				if ((type != W_NORMAL) && (type != W_INVALID)) {
					internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
					internal = &(internalsData.at(internalsData.size()-1));
				} else {
					internal = nullptr;
				}
			}

	Wall(
			glm::vec3 start, glm::vec3 end,
			GLint textureID0,
			WallType type=W_NORMAL, bool* IOPtr=nullptr, float data=0.0f,
			GLint textureID1=-1,
			bool isWorldSpaceX=true, bool isWorldSpaceY=true, bool swapUVXY1=false, bool swapUVXY2=false,
			glm::vec2 textureScale=glm::vec2(1.0f, 1.0f), glm::vec2 textureOffset=glm::vec2(0.0f, 0.0f)
		) : start(glm::vec3(start.x, start.y, std::min(start.z, end.z))), end(glm::vec3(end.x, end.y, std::max(start.z, end.z))),
			originalStart(glm::vec3(start.x, start.y, std::min(start.z, end.z))), originalEnd(glm::vec3(end.x, end.y, std::max(start.z, end.z))),
			type(type), 
			IOPtr(IOPtr), data(data) {
				textureData1s.first = combineTextureData1(textureID0, swapUVXY1);
				if (textureID1 < 0) {
					textureData1s.second = combineTextureData1(textureID0, swapUVXY2);
				} else {
					textureData1s.second = combineTextureData1(textureID1, swapUVXY2);
				}

				textureData2 = combineTextureData2(
					isWorldSpaceX, isWorldSpaceY,
					textureScale, textureOffset
				);

				if ((type != W_NORMAL) && (type != W_INVALID)) {
					internalsData.push_back(std::pair<float, float>(0.0f, 0.0f));
					internal = &(internalsData.at(internalsData.size()-1));
				} else {
					internal = nullptr;
				}
			}
};

struct WallGPU {
	alignas(16) glm::vec3 start;
	alignas(16) glm::vec3 end;
	alignas(8) glm::vec2 direction;
	alignas(4) GLuint textureData1;
	alignas(4) GLuint textureData2;

	WallGPU()
		: start(), end(), direction(),
		  textureData1(), textureData2() {}

	WallGPU(Wall *wall, Player player)
		: start(wall->start), end(wall->end), direction(glm::normalize(glm::vec2(wall->end - wall->start))),
		  textureData2(wall->textureData2) {
			if ((wall->type == W_SWITCH) && (wall->internal->first > 0.0f)) {
				textureData1 = wall->textureData1s.second;
			} else {
				textureData1 = wall->textureData1s.first;
			}
		}
};


struct WallIntersect {
	glm::vec2 position2D;
	glm::vec2 normal2D;
	glm::uint projections;
	glm::uint wallIndexAndXUV;
	float distanceSQ;
	float _padding;

	WallIntersect()
		: position2D(), normal2D(),
		  projections(),
		  wallIndexAndXUV(), distanceSQ() {}
};


struct Displacement {
	std::array<glm::vec3, 3> vertices;
	std::array<glm::vec2, 3> UV;
	glm::vec3 normal;
	GLint textureID;
	DisplacementType type;
	bool* IOPtr;
	float data;
	float internal;

	Displacement() : vertices(), UV(), normal(), textureID(0), type(D_INVALID), data(0.0f), internal(0.0f) {}

	Displacement(
			glm::vec3 vA, glm::vec3 vB, glm::vec3 vC,
			glm::vec2 uvA, glm::vec2 uvB, glm::vec2 uvC,
			GLuint texID, DisplacementType type, bool* ptr, float data
		) : vertices{vA, vB, vC}, UV{uvA, uvB, uvC}, textureID(texID),
			type(type), IOPtr(ptr), data(data), internal(0.0f) {
				normal = glm::normalize(glm::cross(
					vB - vA,
					vC - vA
				));
			}

	Displacement(
			std::array<glm::vec3, 3>& verts, std::array<glm::vec2, 3>& texCoords,
			GLuint texID, DisplacementType type, bool* ptr, float data
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

	DisplacementGPU(Displacement* disp, Player player) {
			for (size_t index=0; index<3; index++) {
				vertices[index] = glm::vec4(disp->vertices.at(index), 0.0f);
				UV[index] = disp->UV.at(index);
			}
			glm::vec3 pDelta = disp->vertices[0] - player.position;
			glm::vec3 normal = disp->normal * -glm::sign(glm::dot(pDelta, disp->normal));
			normal_texID = glm::vec4(normal, disp->textureID);
		}
};


struct Sprite {
	glm::vec3 position;
	float width, height;
	GLint textureID;
	bool collision;
	SpriteType type;
	float mass;
	glm::vec3 velocity;
	glm::vec3 prevPosition;
	float transparency;

	Sprite() : position(), width(), textureID(), type(SPR_INVALID), collision(false), velocity(), prevPosition(), mass(), transparency(1.0f) {}

	Sprite(glm::vec3 position, float width, float height, GLuint textureID, SpriteType type=SPR_DECO, bool collision=true, float mass=1.0f)
		: position(position), width(width), height(height), textureID(textureID), type(type), collision(collision), mass(mass),
		  velocity(0.0f, 0.0f, 0.0f), prevPosition(position), transparency(1.0f) {}
};

struct SpriteGPU {
	alignas(16) glm::vec3 position;
	alignas(4) float width;
	alignas(4) float height;
	alignas(4) GLuint textureID_transparency;
	alignas(4) int screenCentreX;

	SpriteGPU() : position(0.0f, 0.0f, 0.0f), width(0.0f), height(0.0f), textureID_transparency(0), screenCentreX(0) {}

	SpriteGPU(Sprite* sprite, Player player)
		: position(sprite->position),
		  width(sprite->width), height(sprite->height),
		  screenCentreX(getCentreX(sprite->position, player, currentRenderResolution)) {
			textureID_transparency = ((GLuint(sprite->transparency * 65535) & 0xFFFF) << 16) | (sprite->textureID & 0xFFFF);
		  }
};


struct Light {
	glm::vec3 position;
	glm::vec3 colour;
	float intensity;
	bool* IOPtr;

	Light() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), IOPtr(&(constants::C_FALSE)) {}

	Light(glm::vec3 position, glm::vec3 colour, float intensity, bool* IOPtr=&(constants::C_TRUE))
		: position(position), colour(colour), intensity(intensity), IOPtr(IOPtr) {}
};

struct LightGPU {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 colour;
	alignas(4) float intensity;
	alignas(4) bool enabled;
	alignas(4) float _padding;

	LightGPU() : position(0.0f, 0.0f, 0.0f), colour(0.0f, 0.0f, 0.0f), intensity(0.0f), _padding{0.0f} {}

	LightGPU(Light* light, Player player)
		: position(light->position), colour(light->colour),
		  intensity(light->intensity), _padding{0.0f} {
			if (light->IOPtr) {enabled = *(light->IOPtr);}
			else {enabled = true;}
		  }
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

static const std::unordered_map<char, int> chMap = {
	{'|', -3}, {' ', -2},
	{'-', 10}, {'.', 11},
	{'!', 12}, {'?', 13},
	{',', 14}, {'\'', 15},
	{'/', 16}, {':', 17},
	{';', 18}, {'&', 19},
	{'[', 20}, {']', 21},
	{'(', 20}, {')', 21},
	{'^', 22}
};

static int convertTextToIdx(
	const char input,
	std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* symbolNames
) {
	char ch = toupper(input);
	int result;

	auto chMapIt = chMap.find(ch);
	if (chMapIt != chMap.end()) {
		result = chMapIt->second;
	} else {
		auto symNamesIt = std::find(symbolNames->begin(), symbolNames->end(), "symbol_" + std::string(1, ch));
		if (symNamesIt != symbolNames->end()) {
			result = static_cast<int>(std::distance(symbolNames->begin(), symNamesIt));
		} else {
			//Unknown char; show unknown char
			result = 23;
		}
	}

	return result;
}


struct DataSet {
	std::vector<Visplane> visplaneData;
	std::vector<Wall> wallData;
	std::vector<Displacement> displacementData;
	std::vector<Sprite> spriteData;
	std::vector<Light> lightData;
	std::vector<TextObject> textObjectData;
};



struct Ray {
	glm::vec2 position, direction, end;

	Ray(glm::vec2 position, glm::vec2 direction, float len)
		: position(position), direction(direction), end(position + (direction * len)) {}
};



struct UIElement {
	GLuint textureID;
	int* iPtr;
	float* fPtr;
	glm::vec2 position;
	glm::vec2 scale;
	bool* showPtr;

	UIElement() : textureID(), iPtr(nullptr), fPtr(nullptr), position(), scale() {}

	UIElement(glm::vec2 pos, glm::vec2 scale, GLuint textureID, bool* showPtr=nullptr)
		: position(pos), scale(scale), textureID(textureID), iPtr(nullptr), fPtr(nullptr), showPtr(showPtr) {}

	UIElement(glm::vec2 pos, glm::vec2 scale, int* ptr, bool* showPtr=nullptr)
		: position(pos), scale(scale), textureID(0), iPtr(ptr), fPtr(nullptr), showPtr(showPtr) {}

	UIElement(glm::vec2 pos, glm::vec2 scale, float* ptr, bool* showPtr=nullptr)
		: position(pos), scale(scale), textureID(0), fPtr(ptr), iPtr(nullptr), showPtr(showPtr) {}
};

}



inline structs::Player player;


//Mutexes and datasets
inline std::mutex stateSwapMutex;
inline std::atomic<bool> runPhysics = true;
//Data must be synced between the graphics and physics threads.
inline structs::DataSet stateA, stateB;
inline structs::DataSet* physicsData = &stateA;
inline structs::DataSet* graphicsData = &stateB;