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


struct StageData {
	std::string name;
	std::string filePath;

	std::string skyboxTextureName;
	glm::vec3 sunDirection, sunColour;
	
	float gravity;
	float killPlaneZ;

	glm::vec3 playerStartPoint;
	float playerStartAngle;
	float playerStartHealth, playerStartEnergy;


	StageData()
		: name("<NONE>"), filePath(""),
		  skyboxTextureName("fallback-skybox"),
		  sunDirection(0.0f, 0.0f, 1.0f), sunColour(1.0f, 1.0f, 1.0f),
		  gravity(0.486), killPlaneZ(-64.0f),
		  playerStartPoint(0.0f, 0.0f, 0.0f), playerStartAngle(0.0f),
		  playerStartHealth(1.0f), playerStartEnergy(1.0f) {}
};

inline StageData stageData;

inline std::array<bool, constants::MAX_FLAGS> flags;

//Numbers of valid types.
inline size_t validVisplanes;
inline size_t validWalls;
inline size_t validDisplacements;
inline size_t numVisibleVisplanes;
inline size_t numVisibleWalls;
inline size_t numVisibleDisplacements;
inline size_t validSprites;
inline size_t validLights;
inline size_t validTextObjects;
inline size_t validGates;



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