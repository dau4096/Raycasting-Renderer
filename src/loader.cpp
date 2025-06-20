#define TINYOBJLOADER_IMPLEMENTATION
#include "includes.h"
#include "global.h"
#include "utils.h"
#include "tiny_obj_loader.h"
using namespace std;
using namespace utils;
using namespace glm;
using namespace pugi;


namespace xmlFallbackAttribFunc {

static inline glm::vec3 parseVec3(const std::string& str) {
	std::istringstream ss(str);
	glm::vec3 v;
	ss >> v.x >> v.y >> v.z;
	return v;
};

static inline glm::vec2 parseVec2(const std::string& str) {
	std::istringstream ss(str);
	glm::vec2 v;
	ss >> v.x >> v.y;
	return v;
};


int* managePTR(std::string ptrStr, std::array<int, constants::MAX_FLAGS>* flags) {
	std::string ptrStrUpper = strToUpper(ptrStr);
	if ((ptrStrUpper == "TRUE") || (ptrStrUpper == "ALWAYS")) {
		return &(constants::C_TRUE);
	} else if ((ptrStrUpper == "FALSE") || (ptrStrUpper == "NEVER")) {
		return &(constants::C_FALSE);
	}

	try {
		int flagIndex = std::stoi(ptrStr);
		if ((flagIndex < 0) || (flagIndex > (constants::MAX_FLAGS-1))) {
			raise("Pointer string: [" + ptrStr + "] was not an integer flag index, [0 -> " + std::to_string(constants::MAX_FLAGS) + "] (inclusive).");
		}
		return &((*flags)[flagIndex]);


	} catch (const std::invalid_argument& err) {
		raise("Pointer string: [" + ptrStr + "] was not an integer flag index, [0 -> " + std::to_string(constants::MAX_FLAGS) + "] (inclusive).");
	} catch (const std::out_of_range& err) {
		raise("Pointer string: [" + ptrStr + "] was not an integer flag index, [0 -> " + std::to_string(constants::MAX_FLAGS) + "] (inclusive).");
	}
	return &(constants::C_FALSE);
};



static const std::unordered_map<std::string, int> enumMap = {
	//Logic gates 			Walls 					Visplanes 				Sprites 				Displacements
	{"G_INVALID", 0}, 		{"W_INVALID", 0}, 		{"V_INVALID", 0}, 		{"SPR_INVALID", 0},		{"D_INVALID", 0},
	{"G_PASSTHROUGH", 1}, 	{"W_NORMAL", 1},	 	{"V_NORMAL", 1}, 		{"SPR_DECO", 1}, 		{"D_NORMAL", 1},
	{"G_AND", 2}, 			{"W_TRIGGER", 2},	 	{"V_TRIGGER", 2}, 		{"SPR_LIGHT", 2}, 
	{"G_OR", 3}, 			{"W_MOVEV_FAST", 3},	{"V_MOVEV_FAST", 3}, 
	{"G_NOT", 4}, 			{"W_MOVEV_SLOW", 4},	{"V_MOVEV_SLOW", 4}, 
	{"G_XOR", 5}, 			{"W_MOVEH_FAST", 5},	{"V_HURT", 5}, 
	{"G_LATCH", 6}, 		{"W_MOVEH_SLOW", 6},
	{"G_PULSE", 7}, 		{"W_SWITCH", 7}, 
	{"G_TOGGLE", 8},
};

int assignEnum(const std::string& enumStr) {
	auto it = enumMap.find(enumStr);
	if (it != enumMap.end()) return it->second;
	raise("Unknown Enum: " + enumStr);
	return -1;
}

int currentTextureIndex = 0;
int assignTexture(std::string textureStr, std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames) {
	auto begin = textureNames->begin(), end = textureNames->end();
	auto namePTR = std::find(begin, end, textureStr);

	int idx = -1;

	if (namePTR != end) {
		idx = std::distance(begin, namePTR);
	} else {
		if (currentTextureIndex >= display::TEXTURE_ARRAY_MAX_LAYERS) {
			raise("Maximum texture layers reached. Cannot assign more.");
			return -1;
		}
		textureNames->at(currentTextureIndex) = textureStr;
		idx = currentTextureIndex;
		currentTextureIndex++;
	}
	return idx;
}


//Helper functions;

static inline int getInt(const pugi::xml_node& node, std::string attrName, int defaultValue=0) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return attr.as_int();
	}
	return defaultValue;
}

static inline float getFloat(const pugi::xml_node& node, std::string attrName, float defaultValue=0.0f) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return attr.as_float();
	}
	return defaultValue;
}

static inline std::string getString(const pugi::xml_node& node, std::string attrName, std::string defaultValue="") {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return attr.as_string();
	}
	return defaultValue;
}

static inline bool getBool(const pugi::xml_node& node, std::string attrName, bool defaultValue=false) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		std::string attrValue = utils::strToUpper(attr.as_string());
		if (attrValue == "TRUE" || attrValue == "T") {
			return true;
		} else if (attrValue == "FALSE" || attrValue == "F") {
			return false;
		}
	}
	return defaultValue;
}

static inline glm::vec2 getVec2(const pugi::xml_node& node, std::string attrName, glm::vec2 defaultValue=glm::vec2(0.0f, 0.0f)) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return parseVec2(attr.as_string());
	}
	return defaultValue;
}

static inline glm::vec3 getVec3(const pugi::xml_node& node, std::string attrName, glm::vec3 defaultValue=glm::vec3(0.0f, 0.0f, 0.0f)) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return parseVec3(attr.as_string());
	}
	return defaultValue;
}

static inline int getEnum(const pugi::xml_node& node, std::string attrName, int defaultValue=0) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return assignEnum(utils::strToUpper(attr.as_string()));
	}
	return defaultValue;
}

static inline int* getPTR(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::string attrName,
		int* defaultValue=nullptr
	) {
	pugi::xml_attribute attr = node.attribute(attrName);
	if (attr) {
		return managePTR(attr.as_string(), flags);
	}
	return defaultValue;
}

static inline int getTexture(
		const pugi::xml_node& node,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames,
		std::string attrName,
		const char* defaultValue=display::FALLBACK_TEXTURE_PATH
	) {
	pugi::xml_attribute attr = node.attribute(attrName);
	const char* name;
	if (attr) {
		std::string attrValue = attr.as_string();
		name = attrValue.c_str();
	} else {
		name = defaultValue;
	}
	return assignTexture(name, textureNames);
}

}

using namespace xmlFallbackAttribFunc;



namespace models {




static glm::mat4 getModelMat4(glm::vec3 pos, glm::vec3 rot, glm::vec3 scale) {
	glm::mat4 translationMat = glm::mat4(
		1.0f, 	0.0f, 	0.0f, 	pos.x,
		0.0f, 	1.0f, 	0.0f, 	pos.y,
		0.0f, 	0.0f, 	1.0f, 	pos.z,
		0.0f, 	0.0f, 	0.0f, 	1.0f
	);

	float sx = sin(rot.x), cx = cos(rot.x);
	float sy = sin(rot.y), cy = cos(rot.y);
	float sz = sin(rot.z), cz = cos(rot.z);
	glm::mat4 rotationMat = glm::mat4(
		cy*cz, cy*sz, -sy, 0.0f,
		sx*sy*cz-cx*sz, sx*sy*sz+cx*cz, sx*cy, 0.0f,
		cx*sy*cz+sx*sz, cx*sy*sz-sx*cz, cx*cy, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glm::mat4 scaleMat = glm::mat4(
		scale.x,	0.0f, 		0.0f,		0.0f, 
		0.0f, 		scale.y,	0.0f, 		0.0f, 
		0.0f, 		0.0f, 		scale.z,	0.0f, 
		0.0f, 		0.0f, 		0.0f, 		1.0f
	);

	return rotationMat * scaleMat * translationMat;
}


void loadModel(
		const std::string& modelFilePath, 
		std::vector<utils::Displacement>* displacementData,
		int textureID,
		glm::vec3 position=glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 rotation=glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 scale=glm::vec3(1.0f, 1.0f, 1.0f)
	) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;

	glm::mat4 modelMatrix = getModelMat4(position, rotation, scale); //Matrix seems wrong; renders fine when matrix NOT involved.

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, modelFilePath.c_str(), nullptr, true);

	if (!warn.empty()) std::cout << "TinyOBJ warning: " << warn << std::endl;
	if (!ret) return;


	for (const auto& shape : shapes) {
		std::unordered_map<int, int> indexMap;

		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];
			if (fv != 3) {continue; /* Invalid. */}

			Displacement thisDisp;
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shape.mesh.indices[f * fv + v];

				glm::vec4 pos4 = glm::vec4(
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2],
					1.0f
				);

				glm::vec3 pos = glm::vec3(pos4 * modelMatrix);

				glm::vec2 uv = glm::vec2(0.0f, 0.0f);
				if (idx.texcoord_index >= 0) {
					uv = glm::vec2(
						attrib.texcoords[2 * idx.texcoord_index + 0],
						attrib.texcoords[2 * idx.texcoord_index + 1]
					);
				} else {
					textureID = -1;
				}

				thisDisp.vertices[v] = pos;
				thisDisp.UV[v] = uv;
			}
			thisDisp.textureID = textureID;
			thisDisp.type = D_NORMAL;
			thisDisp.IOPtr = nullptr;
			thisDisp.data = 0.0f;

			thisDisp.normal = glm::normalize(glm::cross(
				thisDisp.vertices[1] - thisDisp.vertices[0],
				thisDisp.vertices[2] - thisDisp.vertices[0]
			));
			displacementData->push_back(thisDisp);
		}
	}

	validDisplacements = displacementData->size();
}


void convertOBJIntoDisplacements(
		const pugi::xml_node& node,
		std::vector<utils::Displacement>* displacementData,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	glm::vec3 modelPosition = getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec3 modelRotation = getVec3(node, "rotation", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec3 modelScale = getVec3(node, "scale", glm::vec3(1.0f, 1.0f, 1.0f));
	std::string modelFileName = getString(node, "file", "");
	int textureID = getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME);

	if (!modelFileName.empty()) {
		std::string modelFilePath = "stages/assets-" + utils::strToUpper(userConfig["META_STAGE_NAME"]) + "/" + modelFileName + ".obj";
		loadModel(modelFilePath, displacementData, textureID, modelPosition, modelRotation, modelScale);
	}
}

}


namespace xml {


template<typename T>
std::vector<T> fetchObjectFromXML(
		const pugi::xml_document& doc,
		const std::string& xpath,
		std::function<T(
			const pugi::xml_node&,
			std::array<int, constants::MAX_FLAGS>* flags,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
		)> extractor,
		size_t* numObjects,
		std::array<int, constants::MAX_FLAGS>* flags=nullptr,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames=nullptr
	)
{
	std::vector<T> result{};
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = static_cast<size_t>(nodeList.size());
	*numObjects = count;
	
	for (size_t i=0; i<count; i++) {
		pugi::xml_node node = nodeList[i].node();
		result.push_back(extractor(node, flags, textureNames));
	}
	return result;
}




static inline Visplane extractVisplane(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Visplane visplane = Visplane(
		getVec2(node, "start", glm::vec2(0.0f, 0.0f)),
		getVec2(node, "end", glm::vec2(0.0f, 0.0f)),
		getFloat(node, "height", 0.0f),
		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		static_cast<VisplaneType>(getEnum(node, "type", V_NORMAL)),
		getPTR(node, flags, "IOPtr", nullptr),
		getFloat(node, "extra", 0.0f)
	);
	
	return visplane;
}


static inline Wall extractWall(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Wall wall = Wall(
		getVec3(node, "start", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "end", glm::vec3(0.0f, 0.0f, 0.0f)),
		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		static_cast<WallType>(getEnum(node, "type", W_NORMAL)),
		getPTR(node, flags, "IOPtr", nullptr),
		getFloat(node, "extra", 0.0f)
	);
	
	return wall;
}


static inline Displacement extractDisplacement(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Displacement displacement = Displacement(
		getVec3(node, "aPos", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "bPos", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "cPos", glm::vec3(0.0f, 0.0f, 0.0f)),

		getVec2(node, "aUV", glm::vec2(0.0f, 0.0f)),
		getVec2(node, "bUV", glm::vec2(0.0f, 0.0f)),
		getVec2(node, "cUV", glm::vec2(0.0f, 0.0f)),

		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		static_cast<DisplacementType>(getEnum(node, "type", D_NORMAL)),
		getPTR(node, flags, "IOPtr", nullptr),
		getFloat(node, "extra", 0.0f)
	);
	
	return displacement;
}


static inline Sprite extractSprite(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Sprite sprite = Sprite(
		getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f)),
		getFloat(node, "width", 0.0f),
		getFloat(node, "height", 0.0f),
		getTexture(node, textureNames, "texture", display::FALLBACK_TEXTURE_PATH),
		static_cast<SpriteType>(getEnum(node, "type", SPR_DECO)),
		getBool(node, "collision", false)
	);
	
	return sprite;
}


static inline Light extractLight(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Light light = Light(
		getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "colour", glm::vec3(0.0f, 0.0f, 0.0f)),
		getFloat(node, "intensity", 0.0f),
		getPTR(node, flags, "IOPtr", nullptr)
	);
	
	return light;
}


static inline TextObject extractTextObject(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	TextObject textObject = TextObject(
		getString(node, "text", ""),
		getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f)),
		getInt(node, "scale", 0)
	);
	
	return textObject;
}


static inline LogicGate extractGate(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	LogicGate gate = LogicGate(
		static_cast<GateType>(getEnum(node, "type", G_PASSTHROUGH)),
		getPTR(node, flags, "outputPtr", nullptr),
		getPTR(node, flags, "inputAPtr", nullptr),
		getPTR(node, flags, "inputBPtr", nullptr)
	);
	return gate;
}



void loadModels(
		pugi::xml_document& doc,
		const std::string& xpath,
		std::vector<utils::Displacement>* displacementData,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = static_cast<size_t>(nodeList.size());
	
	for (size_t i=0; i<count; i++) {
		pugi::xml_node node = nodeList[i].node();
		models::convertOBJIntoDisplacements(node, displacementData, textureNames);
	}
}




static inline pugi::xml_node getMetaNode(const pugi::xml_document& doc, std::string subNodeName) {
	std::string xpath = "//meta/" + subNodeName;
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = static_cast<size_t>(nodeList.size());
	if (count != 1) {
		utils::print("Unknown " + subNodeName + " metanode found.");
	}
	return nodeList[0].node();
}


static inline float handlePlayerHEString(std::string inputSTR, float maxValue) {
	if (strToUpper(inputSTR) == "MAX") {
		return maxValue;
	} else if (strToUpper(inputSTR) == "MIN") {
		return 1.0f;
	}
	try {
		return std::stof(inputSTR);
	} catch (const std::invalid_argument) {
		raise("Unable to convert " + inputSTR + " to a floating-point value.");
	}
	return 0.0f;
}


void retrieveStageMetaData(const pugi::xml_document& doc, utils::Player* player) {
	//Sky
	pugi::xml_node skyNode = getMetaNode(doc, "sky");
	stageData.skyboxTextureName = getString(skyNode, "skyboxTexture", std::string(initial::FALLBACK_SKYBOX_NAME));


	//Texture
	pugi::xml_node textureNode = getMetaNode(doc, "texture");
	stageData.textureScale = getVec2(textureNode, "scale", initial::TEXTURE_SCALE);
	stageData.textureOffset = getVec3(textureNode, "offset", initial::TEXTURE_OFFSET);


	//Sun
	pugi::xml_node sunNode = getMetaNode(doc, "sun");
	stageData.sunDirection = getVec3(sunNode, "sunDirection", initial::SUN_DIRECTION);
	float sunIntensity = getFloat(sunNode, "sunIntensity", initial::SUN_INTENSITY);
	stageData.sunColour = getVec3(sunNode, "sunColour", initial::SUN_COLOUR) * sunIntensity;


	//Physics
	pugi::xml_node physNode = getMetaNode(doc, "physics");
	stageData.gravity = getFloat(physNode, "gravity", initial::GRAVITY_ACCEL);
	stageData.killPlaneZ = getFloat(physNode, "killPlaneZ", initial::KILL_PLANE_Z);


	//Player
	pugi::xml_node playerNode = getMetaNode(doc, "player");
	stageData.playerStartPoint = getVec3(playerNode, "startPoint", initial::PLAYER_START_POSITION);
	stageData.playerStartAngle = getFloat(playerNode, "startAngle", initial::PLAYER_START_VANGLE);

	std::string startHealthStr = getString(playerNode, "initialHealth", "MAX");
	stageData.playerStartHealth = handlePlayerHEString(startHealthStr, playerConfig::PLAYER_MAX_HEALTH);

	std::string startEnergyStr = getString(playerNode, "initialEnergy", "MAX");
	stageData.playerStartEnergy = handlePlayerHEString(startEnergyStr, playerConfig::PLAYER_MAX_ENERGY);

	*player = utils::Player();
}



void fetchBindingsFromXML(const pugi::xml_document& doc) {
	const char* xpath = "//keybinds/bind";
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath);
	size_t count = static_cast<size_t>(nodeList.size());
	
	std::string functionString, keyString;
	for (size_t i = 0; i < count; ++i) {
		pugi::xml_node node = nodeList[i].node();

		functionString = strToUpper(node.attribute("function").as_string());
		if (userBindings.find(functionString) == userBindings.end()) {
			raise("Unknown binding function: " + functionString);
		}

		keyString = strToUpper(node.attribute("key").as_string());
		if (keyNameToGLFW.find(keyString) != keyNameToGLFW.end()) {
			userBindings.at(functionString) = keyNameToGLFW.at(keyString);
		} else {
			raise("Unknown Key: " + keyString + " for binding function: " + functionString);
		}
	}
}

void fetchConfigsFromXML(const pugi::xml_document& doc) {
	const char* xpath = "//config/option";
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath);
	size_t count = static_cast<size_t>(nodeList.size());
	
	std::string functionString, valueString;
	for (size_t i = 0; i < count; ++i) {
		pugi::xml_node node = nodeList[i].node();

		functionString = strToUpper(node.attribute("function").as_string());
		if (userConfig.find(functionString) == userConfig.end()) {
			raise("Unknown config function: " + functionString);
		}

		valueString = strToUpper(node.attribute("value").as_string());
		if (valueString != "") {
			userConfig.at(functionString) = valueString;
		} else {
			raise("Invalid value for: " + functionString);
		}
	}
}

}


static inline std::unordered_map<std::string, glm::ivec2> resolutionMap = {
	{"TERRIBLE", glm::ivec2(64, 36)},
	{"AWFUL", glm::ivec2(256, 144)},
	{"CALCULATOR", glm::ivec2(384, 216)},
	{"DS", glm::ivec2(400, 240)},
	{"LOW", glm::ivec2(640, 360)},
	{"MEDIUM", glm::ivec2(960, 540)}, {"", glm::ivec2(960, 540)}, //Blank option.
	{"HIGH", glm::ivec2(1280, 720)},
	{"AMAZING", glm::ivec2(1920, 1080)}
};

static inline std::unordered_map<std::string, int> debugMap = {
	{"", 0}, {"NONE", 0},
	{"UV", 1}, {"TEXTURE_UV", 1},
	{"NORMALS", 2}, {"SURFACE_NORMALS", 2},
};

static inline std::unordered_map<std::string, int> texMipMap = {
	{"", 0}, {"HIGH", 0},
	{"MEDIUM", 1},
	{"LOW", 2},
	{"AWFUL", 3},
	{"TERRIBLE", 4}
};


namespace loader {


void loadStage(
		const std::string& stageName, utils::Player* player,
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::vector<utils::Displacement>* displacementData,
		std::vector<utils::Sprite>* spriteData,
		std::vector<utils::Light>* lightData,
		std::vector<utils::TextObject>* textObjectData,
		std::vector<utils::LogicGate>* logicGates,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string filePath = "stages/" + stageName + ".xml";
	std::string XMLSrc = utils::readFile(filePath);


	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}
	
	*visplaneData = xml::fetchObjectFromXML<utils::Visplane>(doc, "//visplanes/visplane", xml::extractVisplane, &validVisplanes, flags, textureNames);
	*wallData = xml::fetchObjectFromXML<utils::Wall>(doc, "//walls/wall", xml::extractWall, &validWalls, flags, textureNames);
	*displacementData = xml::fetchObjectFromXML<utils::Displacement>(doc, "//displacements/displacement", xml::extractDisplacement, &validDisplacements, flags, textureNames);
	*spriteData	= xml::fetchObjectFromXML<utils::Sprite>(doc, "//sprites/sprite", xml::extractSprite, &validSprites, nullptr, textureNames);
	*lightData = xml::fetchObjectFromXML<utils::Light>(doc, "//lights/light", xml::extractLight, &validLights, nullptr, nullptr);
	*textObjectData = xml::fetchObjectFromXML<utils::TextObject>(doc, "//objects/textObj", xml::extractTextObject, &validTextObjects, nullptr, nullptr);
	*logicGates	= xml::fetchObjectFromXML<utils::LogicGate>(doc, "//logicGates/logic", xml::extractGate, &validGates, flags, nullptr);

	xml::loadModels(doc, "//models/model", displacementData, textureNames);


	stageData.name = stageName;
	stageData.filePath = filePath;
	xml::retrieveStageMetaData(doc, player);
}


void loadBindings() {
	const std::string filePath = "userConfig.xml";
	std::string XMLSrc = utils::readFile(filePath);

	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}

	xml::fetchBindingsFromXML(doc);
	xml::fetchConfigsFromXML(doc);


	//Handle render quality setting.
	/*
	std::string renderQuality = userConfig["VIEW_RENDER_RESOLUTION_QUALITY"];
	auto resolutionIt = resolutionMap.find(renderQuality);
	if (resolutionIt != resolutionMap.end()) {
		desiredRenderResolution = resolutionMap[renderQuality];
	} else {
		std::cout << ("Invalid render resolution quality: " + renderQuality) << std::endl << "Expected one of:";
		for (auto pair : resolutionMap) {
			if (pair.first.empty()) {continue; / Blank option /}
			std::cout << std::endl << pair.first << " for [" << pair.second.x << " x " << pair.second.y << "]";
		}
		desiredRenderResolution = resolutionMap["LOW"];
	}
	*/

	std::string mode = userConfig["META_DEBUG_MODE"];
	auto debugIt = debugMap.find(mode);
	if (debugIt != debugMap.end()) {
		userConfig["META_DEBUG_MODE"] = std::to_string(debugMap[mode]);
	} else {
		std::cout << ("Invalid debug mode: " + mode) << std::endl << "Expected one of:";
		for (auto pair : debugMap) {
			if (pair.first.empty()) {continue; /* Blank option */}
			std::cout << std::endl << pair.first;
		}
		userConfig["META_DEBUG_MODE"] = std::to_string(debugMap["NONE"]);
	}

	std::string quality = userConfig["VIEW_TEXTURE_QUALITY"];
	auto mipIt = texMipMap.find(quality);
	if (mipIt != texMipMap.end()) {
		userConfig["VIEW_TEXTURE_QUALITY"] = std::to_string(texMipMap[quality]);
	} else {
		std::cout << ("Invalid texture quality: " + quality) << std::endl << "Expected one of:";
		for (auto pair : texMipMap) {
			if (pair.first.empty()) {continue; /* Blank option */}
			int mipLevel = pair.second;
			glm::ivec2 mipRes = glm::vec2(display::TEXTURE_RESOLUTION) / static_cast<float>(pow(2, mipLevel));
			std::cout << std::endl << pair.first << " for [" << mipRes.x << " x " << mipRes.y << "]";
		}
		userConfig["VIEW_TEXTURE_QUALITY"] = std::to_string(texMipMap["HIGH"]);
	}

	if (utils::configToBool("META_SHOW_CONSOLE")) {
		utils::showConsole();
	} else {
		utils::hideConsole();
	}
}

}