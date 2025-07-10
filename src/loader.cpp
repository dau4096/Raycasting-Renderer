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



std::unordered_map<std::string, bool*> flagList;
size_t flagIndex = 0;
bool* managePTR(std::string ptrStr, std::array<bool, constants::MAX_FLAGS>* flags) {
	std::string ptrStrUpper = strToUpper(ptrStr);
	if ((ptrStrUpper == "TRUE") || (ptrStrUpper == "ALWAYS")) {
		return &(constants::C_TRUE);
	} else if ((ptrStrUpper == "FALSE") || (ptrStrUpper == "NEVER")) {
		return &(constants::C_FALSE);
	}

	auto it = flagList.find(ptrStrUpper);
	if (it == flagList.end()) {
		bool* ptr = &(*flags)[flagIndex++];
		flagList[ptrStrUpper] = ptr;
		return ptr;
	}
	return it->second;
};



static const std::unordered_map<std::string, int> enumMap = {
	//Walls 				Visplanes				Logic Gates				Sprites 				Displacements
	{"W_INVALID", 0}, 		{"V_INVALID", 0}, 		{"G_INVALID", 0}, 		{"SPR_INVALID", 0},		{"D_INVALID", 0},
	{"W_NORMAL", 1},	 	{"V_NORMAL", 1}, 		{"G_PASSTHROUGH", 1}, 	{"SPR_DECO", 1}, 		{"D_NORMAL", 1},
	{"W_TRIGGER", 2},	 	{"V_TRIGGER", 2}, 		{"G_AND", 2}, 			{"SPR_LIGHT", 2}, 
	{"W_MOVED_FAST", 3},	{"V_MOVEX_FAST", 3}, 	{"G_OR", 3}, 			 
	{"W_MOVED_SLOW", 4},	{"V_MOVEX_SLOW", 4}, 	{"G_NOT", 4}, 			 
	{"W_MOVEN_FAST", 5},	{"V_MOVEY_FAST", 5}, 	{"G_XOR", 5}, 			 
	{"W_MOVEN_SLOW", 6},	{"V_MOVEY_SLOW", 6}, 	{"G_LATCH", 6}, 		
	{"W_MOVEZ_FAST", 7},	{"V_MOVEZ_FAST", 7}, 	{"G_PULSE", 7}, 		
	{"W_MOVEZ_SLOW", 8},	{"V_MOVEZ_SLOW", 8}, 	{"G_TOGGLE", 8},		
	{"W_SWITCH", 9},		{"V_HURT", 9},
	{"W_PASSTHROUGH", 10},	{"V_PASSTHROUGH", 10},
	{"W_DOORZ", 11},		{"V_NODRAW", 11},
	{"W_DOORSWING", 12},	{"V_TELEPORT", 12},
	{"W_NODRAW", 13},		{"V_CONVEY", 13},
};

int assignEnum(const std::string& enumStr) {
	auto it = enumMap.find(enumStr);
	if (it != enumMap.end()) {return it->second;}
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

static inline bool* getPTR(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::string attrName,
		bool* defaultValue=nullptr
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
	const char* texname;
	if (attr) {
		std::string attrValue = attr.as_string();
		texname = attrValue.c_str();
	} else {
		texname = defaultValue;
	}
	return assignTexture(texname, textureNames);
}


static std::unordered_map<std::string, std::pair<size_t, size_t>> indexMap;
float assignExtra(const pugi::xml_node& node, size_t index=0) {
	if (utils::strToUpper(node.attribute("type").value()) == "V_TELEPORT") {
		std::string teleflag = node.attribute("extra").as_string();
		auto it = indexMap.find(teleflag);
		if (it == indexMap.end()) {
			indexMap[teleflag] = std::pair<size_t, size_t>{index, 0};
		} else {
			indexMap[teleflag].second = index;
		}
		return 0.0f;
	}
	return node.attribute("extra").as_float();
}

static inline float getExtra(const pugi::xml_node& node, float defaultValue=0.0f, size_t index=0) {
	pugi::xml_attribute attr = node.attribute("extra");
	if (attr) {
		return assignExtra(node, index);
	}
	return defaultValue;
}



void processTeleporterPartners(std::vector<utils::Visplane>* visplaneData) {
	for (std::pair<std::string, std::pair<size_t, size_t>> pair : indexMap) {
		visplaneData->at(pair.second.first).data = pair.second.second;
		visplaneData->at(pair.second.second).data = pair.second.first;
	}
}


}

using namespace xmlFallbackAttribFunc;



namespace macros {
//Any non-elementary type shorthand nodes in the XML file which can be decomposed into primative types such as walls.

void fetchMacroFromXML(
		const pugi::xml_document& doc,
		const std::string& xpath,
		std::function<void(
			pugi::xml_node node,
			std::vector<utils::Visplane>* visplaneData,
			std::vector<utils::Wall>* wallData,
			std::array<bool, constants::MAX_FLAGS>* flags,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
		)> extractor,
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::array<bool, constants::MAX_FLAGS>* flags=nullptr,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames=nullptr
	)
{
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = static_cast<size_t>(nodeList.size());
	
	for (size_t i=0; i<count; i++) {
		pugi::xml_node node = nodeList[i].node();
		extractor(node, visplaneData, wallData, flags, textureNames);
	}
}

void extractCuboid(
		pugi::xml_node node,
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	glm::vec3 lowerCorner = getVec3(node, "start", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec3 upperCorner = getVec3(node, "end", glm::vec3(0.0f, 0.0f, 0.0f));
	GLuint sideTexture = getTexture(node, textureNames, "sideTexture", initial::FALLBACK_TEXTURE_NAME);
	GLuint topTexture = getTexture(node, textureNames, "topTexture", initial::FALLBACK_TEXTURE_NAME);
	GLuint lowTexture = getTexture(node, textureNames, "bottomTexture", initial::FALLBACK_TEXTURE_NAME);


	int intType = getEnum(node, "type", 1); //V_NORMAL, W_NORMAL and D_NORMAL are all integer value 1.
	VisplaneType vType = static_cast<VisplaneType>(intType);
	WallType wType = static_cast<WallType>(intType);
	float extra = getFloat(node, "data", 0.0f);
	bool* ptr = getPTR(node, flags, "flag");

	glm::bvec3 worldSpaceTextures = glm::bvec3(
		getBool(node, "useWorldUVX", true),
		getBool(node, "useWorldUVY", true),
		getBool(node, "useWorldUVZ", true)
	);

	glm::vec3 textureScale = getVec3(node, "textureScale", glm::vec3(1.0f, 1.0f, 1.0f));
	glm::vec3 textureOffset = getVec3(node, "textureOffset", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec2 wTexScale = glm::vec2(textureScale.x, textureScale.z);
	glm::vec2 wTexOffset = glm::vec2(textureOffset.x, textureOffset.z);


	if (getBool(node, "hasTop", true)) {
		visplaneData->push_back(utils::Visplane(
			glm::vec2(lowerCorner), glm::vec2(upperCorner), upperCorner.z,
			topTexture, vType, ptr, extra,
			worldSpaceTextures.x, worldSpaceTextures.y,
			glm::vec2(textureScale), glm::vec2(textureOffset)
		));
		validVisplanes++;
	}
	if (getBool(node, "hasBottom", true)) {
		visplaneData->push_back(utils::Visplane(
			glm::vec2(lowerCorner), glm::vec2(upperCorner), lowerCorner.z,
			lowTexture, vType, ptr, extra,
			worldSpaceTextures.x, worldSpaceTextures.y,
			glm::vec2(textureScale), glm::vec2(textureOffset)
		));
		validVisplanes++;
	}

	std::vector<utils::Wall> newWData = {
		utils::Wall(
			lowerCorner, glm::vec3(lowerCorner.x, upperCorner.y, upperCorner.z),
			sideTexture, wType, ptr, extra,
			-1, worldSpaceTextures.x, worldSpaceTextures.z,
			wTexScale, wTexOffset
		),
		utils::Wall(
			glm::vec3(lowerCorner.x, upperCorner.y, lowerCorner.z), upperCorner,
			sideTexture, wType, ptr, extra,
			-1, worldSpaceTextures.x, worldSpaceTextures.z,
			wTexScale, wTexOffset
		),
		utils::Wall(
			lowerCorner, glm::vec3(upperCorner.x, lowerCorner.y, upperCorner.z),
			sideTexture, wType, ptr, extra,
			-1, worldSpaceTextures.x, worldSpaceTextures.z,
			wTexScale, wTexOffset
		),
		utils::Wall(
			glm::vec3(upperCorner.x, lowerCorner.y, lowerCorner.z), upperCorner,
			sideTexture, wType, ptr, extra,
			-1, worldSpaceTextures.x, worldSpaceTextures.z,
			wTexScale, wTexOffset
		),
	};
	utils::combineVectors(wallData, newWData);
	validWalls += 4;
}


void extractStairs(
		pugi::xml_node node,
		std::vector<utils::Visplane>* visplaneData,
		std::vector<utils::Wall>* wallData,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	glm::vec3 lowerCorner = getVec3(node, "start", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec3 upperCorner = getVec3(node, "end", glm::vec3(0.0f, 0.0f, 0.0f));
	bool lowerIsLower = lowerCorner.z < upperCorner.z;
	float zDelta = upperCorner.z - lowerCorner.z;
	int numStairs = static_cast<int>(std::max(1, int(std::ceil(abs(zDelta) / constants::MAX_STEP_HEIGHT))));

	GLuint sideTexture = getTexture(node, textureNames, "sideTexture", initial::FALLBACK_TEXTURE_NAME);
	GLuint stepsTexture = getTexture(node, textureNames, "stepsTexture", initial::FALLBACK_TEXTURE_NAME);

	bool stairsStatic = getBool(node, "static", true);
	VisplaneType vType;
	WallType wType;
	if (stairsStatic) {
		vType = V_NORMAL;
		wType = W_NORMAL;
	} else {
		if (getBool(node, "slowMovement", true)) {
			vType = V_MOVEZ_SLOW;
			wType = W_MOVEZ_SLOW;
		} else {
			vType = V_MOVEZ_FAST;
			wType = W_MOVEZ_FAST;
		}
	}
	bool* ptr = getPTR(node, flags, "flag");

	bool hasEnd = getBool(node, "hasEndWall", true);
	bool hasSides = getBool(node, "hasSideWalls", true);
	bool hasConnectors = getBool(node, "hasConnectingWalls", true);

	glm::bvec3 worldSpaceTextures = glm::bvec3(
		getBool(node, "useWorldUVX", true),
		getBool(node, "useWorldUVY", true),
		getBool(node, "useWorldUVZ", true)
	);

	glm::vec3 textureScale = getVec3(node, "textureScale", glm::vec3(1.0f, 1.0f, 1.0f));
	glm::vec3 textureOffset = getVec3(node, "textureOffset", glm::vec3(0.0f, 0.0f, 0.0f));
	glm::vec2 wTexScale = glm::vec2(textureScale.x, textureScale.z);
	glm::vec2 wTexOffset = glm::vec2(textureOffset.x, textureOffset.z);



	glm::vec2 stairMin = glm::vec2(std::min(lowerCorner.x, upperCorner.x), std::min(lowerCorner.y, upperCorner.y));
	glm::vec2 stairMax = glm::vec2(std::max(lowerCorner.x, upperCorner.x), std::max(lowerCorner.y, upperCorner.y));
	glm::vec2 stairDelta = stairMax - stairMin;
	if (abs(zDelta) < constants::MAX_STEP_HEIGHT) {
		//Flat floor, delta is shorter than 1 step.
		visplaneData->push_back(utils::Visplane(
			stairMin, stairMax, (lowerCorner.z + upperCorner.z) / 2.0f, //Average the Z.
			stepsTexture, V_NORMAL, nullptr, 0.0f,
			worldSpaceTextures.x, worldSpaceTextures.y,
			glm::vec2(textureScale), glm::vec2(textureOffset)
		));
		validVisplanes++;
		return;
	}

	float stepDelta = zDelta / float(numStairs);
	float stepHeight = lowerCorner.z;
	if (abs(stairDelta.y) > abs(stairDelta.x)) {
		//In the Y direction.
		float stairWidth = stairDelta.y / float(numStairs);
		float currentY;
		if (((zDelta > 0.0f) && lowerIsLower) || ((zDelta <= 0.0f) && !lowerIsLower)) {
			//+Y direction
			currentY = stairMin.y;
		} else {
			//-Y direction
			currentY = stairMax.y;
		}

		for (size_t stepIdx=0; stepIdx<numStairs; stepIdx++) {
			//Visplane step
			visplaneData->push_back(utils::Visplane(
				glm::vec2(lowerCorner.x, currentY), glm::vec2(upperCorner.x, currentY + stairWidth), stepHeight,
				stepsTexture, vType, ptr, -0.01f-stepHeight,
				worldSpaceTextures.x, worldSpaceTextures.y,
				glm::vec2(textureScale), glm::vec2(textureOffset)

			));
			validVisplanes++;

			//Side walls
			if (hasSides) {
				wallData->push_back(utils::Wall(
					glm::vec3(lowerCorner.x, currentY, (lowerIsLower) ? lowerCorner.z : upperCorner.z), glm::vec3(lowerCorner.x, currentY + stairWidth, stepHeight),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				wallData->push_back(utils::Wall(
					glm::vec3(upperCorner.x, currentY, (lowerIsLower) ? lowerCorner.z : upperCorner.z), glm::vec3(upperCorner.x, currentY + stairWidth, stepHeight),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls += 2;
			}

			//Mid-step connecting wall
			currentY += stairWidth;
			if (hasConnectors) {
				wallData->push_back(utils::Wall(
					glm::vec3(lowerCorner.x, currentY, stepHeight), glm::vec3(upperCorner.x, currentY, stepHeight + stepDelta),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls++;
			}

			stepHeight += stepDelta;

			if (hasEnd) {
				wallData->push_back(utils::Wall(
					glm::vec3(lowerCorner.x, (lowerIsLower) ? upperCorner.y : lowerCorner.y, lowerCorner.z),
					glm::vec3(upperCorner.x, (lowerIsLower) ? upperCorner.y : lowerCorner.y, upperCorner.z),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls++;
			}
		}
	} else {
		//In the X direction.
		float stairWidth = stairDelta.x / float(numStairs);
		float currentX;
		if (((zDelta > 0.0f) && lowerIsLower) || ((zDelta <= 0.0f) && !lowerIsLower)) {
			//+X direction
			currentX = stairMin.x;
		} else {
			//-X direction
			currentX = stairMax.x;
		}

		for (size_t stepIdx=0; stepIdx<numStairs; stepIdx++) {
			visplaneData->push_back(utils::Visplane(
				glm::vec2(currentX, lowerCorner.y), glm::vec2(currentX + stairWidth, upperCorner.y), stepHeight,
				stepsTexture, vType, ptr, -0.01f-stepHeight,
				worldSpaceTextures.x, worldSpaceTextures.y,
				glm::vec2(textureScale), glm::vec2(textureOffset)

			));
			validVisplanes++;

			//Side walls
			if (hasSides) {
				wallData->push_back(utils::Wall(
					glm::vec3(currentX, lowerCorner.y, (lowerIsLower) ? lowerCorner.z : upperCorner.z), glm::vec3(currentX + stairWidth, lowerCorner.y, stepHeight),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				wallData->push_back(utils::Wall(
					glm::vec3(currentX, upperCorner.y, (lowerIsLower) ? lowerCorner.z : upperCorner.z), glm::vec3(currentX + stairWidth, upperCorner.y, stepHeight),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls += 2;
			}

			//Mid-step connecting wall
			currentX += stairWidth;
			if (hasConnectors) {
				wallData->push_back(utils::Wall(
					glm::vec3(currentX, lowerCorner.y, stepHeight), glm::vec3(currentX, upperCorner.y, stepHeight + stepDelta),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls++;
			}

			stepHeight += stepDelta;

			if (hasEnd) {
				wallData->push_back(utils::Wall(
					glm::vec3((lowerIsLower) ? upperCorner.x : lowerCorner.x, lowerCorner.y, lowerCorner.z),
					glm::vec3((lowerIsLower) ? upperCorner.x : lowerCorner.x, upperCorner.y, upperCorner.z),
					sideTexture, wType, ptr, -0.01f-stepHeight,
					-1, worldSpaceTextures.x, worldSpaceTextures.z,
					wTexScale, wTexOffset
				));
				validWalls++;
			}
		}
	}
}

}




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

	glm::mat4 modelMatrix = getModelMat4(position, rotation, scale);

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, modelFilePath.c_str(), nullptr, true);

	if (!warn.empty()) std::cout << "TinyOBJ warning: " << warn << std::endl;
	if (!ret) return;


	for (const auto& shape : shapes) {
		std::unordered_map<int, int> indexMap;
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int fv = shape.mesh.num_face_vertices[f];
			if (fv != 3) {
				index_offset += fv;
				continue;
			}

			Displacement thisDisp;
			for (size_t v = 0; v < 3; v++) {
				tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

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

			index_offset += fv;
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

inline size_t objectIndex = 0;
template<typename T>
std::vector<T> fetchObjectFromXML(
		const pugi::xml_document& doc,
		const std::string& xpath,
		std::function<T(
			const pugi::xml_node&,
			std::array<bool, constants::MAX_FLAGS>* flags,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
		)> extractor,
		size_t* numObjects,
		std::array<bool, constants::MAX_FLAGS>* flags=nullptr,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames=nullptr
	)
{
	std::vector<T> result{};
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = static_cast<size_t>(nodeList.size());
	*numObjects = count;
	
	for (objectIndex=0; objectIndex<count; objectIndex++) {
		pugi::xml_node node = nodeList[objectIndex].node();
		result.push_back(extractor(node, flags, textureNames));
	}
	return result;
}




static inline Visplane extractVisplane(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	VisplaneType type = static_cast<VisplaneType>(getEnum(node, "type", V_NORMAL));
	Visplane visplane = Visplane(
		getVec2(node, "start", glm::vec2(0.0f, 0.0f)),
		getVec2(node, "end", glm::vec2(0.0f, 0.0f)),
		getFloat(node, "height", 0.0f),
		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		type,
		getPTR(node, flags, "flag", nullptr),
		getExtra(node, 0.0f, objectIndex),
		getBool(node, "useWorldUVX", true),
		getBool(node, "useWorldUVY", true),
		getVec2(node, "textureScale", glm::vec2(1.0f, 1.0f)),
		getVec2(node, "textureOffset", glm::vec2(0.0f, 0.0f)),
		getFloat(node, "exitDirection", constants::INF)
	);

	if (type == V_CONVEY) {
		glm::vec2 direction = glm::normalize(getVec2(node, "direction", glm::vec2(0.0f, 1.0f)));
		visplane.internal->first = direction.x;
		visplane.internal->second = direction.y;
	}
	
	return visplane;
}


static inline Wall extractWall(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Wall wall = Wall(
		getVec3(node, "start", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "end", glm::vec3(0.0f, 0.0f, 0.0f)),
		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		static_cast<WallType>(getEnum(node, "type", W_NORMAL)),
		getPTR(node, flags, "flag", nullptr),
		getFloat(node, "extra", 0.0f),
		getTexture(node, textureNames, "altTexture", initial::FALLBACK_TEXTURE_NAME),
		getBool(node, "useWorldUVX", true),
		getBool(node, "useWorldUVY", true),
		getVec2(node, "textureScale", glm::vec2(1.0f, 1.0f)),
		getVec2(node, "textureOffset", glm::vec2(0.0f, 0.0f))
	);
	
	return wall;
}


static inline Displacement extractDisplacement(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Displacement displacement = Displacement(
		getVec3(node, "aPos", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "bPos", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "cPos", glm::vec3(0.0f, 0.0f, 0.0f)),

		getVec2(node, "aUV", glm::vec2(0.0f, 0.0f)),
		getVec2(node, "bUV", glm::vec2(1.0f, 0.0f)),
		getVec2(node, "cUV", glm::vec2(1.0f, 1.0f)),

		getTexture(node, textureNames, "texture", initial::FALLBACK_TEXTURE_NAME),
		static_cast<DisplacementType>(getEnum(node, "type", D_NORMAL)),
		getPTR(node, flags, "flag", nullptr),
		getFloat(node, "extra", 0.0f)
	);
	
	return displacement;
}


static inline Sprite extractSprite(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Sprite sprite = Sprite(
		getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f)),
		getFloat(node, "width", 1.0f),
		getFloat(node, "height", 1.0f),
		getTexture(node, textureNames, "texture", display::FALLBACK_TEXTURE_PATH),
		static_cast<SpriteType>(getEnum(node, "type", SPR_DECO)),
		getBool(node, "collision", false)
	);
	
	return sprite;
}


static inline Light extractLight(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {

	Light light = Light(
		getVec3(node, "position", glm::vec3(0.0f, 0.0f, 0.0f)),
		getVec3(node, "colour", glm::vec3(1.0f, 1.0f, 1.0f)),
		getFloat(node, "intensity", 1.0f),
		getPTR(node, flags, "flag", nullptr)
	);
	
	return light;
}


static inline TextObject extractTextObject(
		const pugi::xml_node& node,
		std::array<bool, constants::MAX_FLAGS>* flags,
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
		std::array<bool, constants::MAX_FLAGS>* flags,
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


static std::unordered_map<std::string, glm::ivec2> resolutionMap = {
	{"TERRIBLE", glm::ivec2(64, 36)},
	{"AWFUL", glm::ivec2(256, 144)},
	{"CALCULATOR", glm::ivec2(384, 216)},
	{"DS", glm::ivec2(400, 240)},
	{"LOW", glm::ivec2(640, 360)},
	{"MEDIUM", glm::ivec2(960, 540)}, {"", glm::ivec2(960, 540)}, //Blank option.
	{"HIGH", glm::ivec2(1280, 720)},
	{"AMAZING", glm::ivec2(1920, 1080)}
};

static std::unordered_map<std::string, int> debugMap = {
	{"", 0}, {"NONE", 0},
	{"UV", 1}, {"TEXTURE_UV", 1},
	{"NORMALS", 2}, {"SURFACE_NORMALS", 2},
};

static std::unordered_map<std::string, int> texMipMap = {
	{"", 0}, {"HIGH", 0},
	{"MEDIUM", 1},
	{"LOW", 2},
	{"AWFUL", 3},
	{"TERRIBLE", 4}
};

static std::unordered_map<std::string, float> shadowQualityMap = {
	{"FULL", 1.0f}, {"1/1", 1.0f},
	{"HALF", 0.5f}, {"1/2", 0.5f}, {"", 0.5f},
	{"QUARTER", 0.25f}, {"1/4", 0.25f},
	{"EIGHTH", 0.125f}, {"1/8", 0.125f}
};


//Has pointer
template<typename T>
static inline void setConfigFromStringOptionsMap(
		std::string configName,
		std::unordered_map<std::string, T>* map,
		const std::string& defaultValue,
		T* outPTR
	) {
	std::string keyString = userConfig[configName];
	auto it = map->find(keyString);
	if (it != map->end()) {
		*outPTR = it->second;
	} else {
		std::cout << ("Invalid config value: " + keyString) << std::endl << "Expected one of:";
		for (const auto& pair : *map) {
			if (pair.first.empty()) {continue; /* Blank option */}
			if constexpr (std::is_same_v<T, glm::ivec2>) {
				std::cout << " for [" << pair.second.x << " x " << pair.second.y << "]";
			} else if constexpr (std::is_same_v<T, glm::ivec3>) {
				std::cout << " for [" << pair.second.x << " x " << pair.second.y << " x " << pair.second.z << "]";
			} else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_arithmetic_v<T>) {
				std::cout << " for " << pair.second;
			} else {
				// Fallback for other types
				std::cout << " (unprintable value type)";
			}
		}

		auto fallback = map->at(defaultValue);
		*outPTR = fallback;
	}

}

//No pointer
template<typename T>
static inline void setConfigFromStringOptionsMap(
		std::string configName,
		std::unordered_map<std::string, T>* map,
		const std::string& defaultValue
	) {
	std::string keyString = userConfig[configName];
	auto it = map->find(keyString);
	if (it != map->end()) {
		userConfig[configName] = std::to_string(it->second);
	} else {
		std::cout << ("Invalid config value: " + keyString) << std::endl << "Expected one of:";
		for (const auto& pair : *map) {
			if (pair.first.empty()) {continue; /* Blank option */}
			if constexpr (std::is_same_v<T, glm::ivec2>) {
				std::cout << " for [" << pair.second.x << " x " << pair.second.y << "]";
			} else if constexpr (std::is_same_v<T, glm::ivec3>) {
				std::cout << " for [" << pair.second.x << " x " << pair.second.y << " x " << pair.second.z << "]";
			} else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_arithmetic_v<T>) {
				std::cout << " for " << pair.second;
			} else {
				// Fallback for other types
				std::cout << " (unprintable value type)";
			}
		}

		auto fallback = map->at(defaultValue);
		userConfig[configName] = std::to_string(fallback);
	}

}


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
		std::array<bool, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string filePath = "stages/" + stageName + ".xml";
	std::string XMLSrc = utils::readFile(filePath);


	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}
	
	*visplaneData = xml::fetchObjectFromXML<utils::Visplane>(doc, "//environment/visplane", xml::extractVisplane, &validVisplanes, flags, textureNames);
	*wallData = xml::fetchObjectFromXML<utils::Wall>(doc, "//environment/wall", xml::extractWall, &validWalls, flags, textureNames);
	*displacementData = xml::fetchObjectFromXML<utils::Displacement>(doc, "//environment/displacement", xml::extractDisplacement, &validDisplacements, flags, textureNames);
	*spriteData	= xml::fetchObjectFromXML<utils::Sprite>(doc, "//objects/sprite", xml::extractSprite, &validSprites, nullptr, textureNames);
	*lightData = xml::fetchObjectFromXML<utils::Light>(doc, "//objects/light", xml::extractLight, &validLights, nullptr, nullptr);
	*textObjectData = xml::fetchObjectFromXML<utils::TextObject>(doc, "//objects/textObj", xml::extractTextObject, &validTextObjects, nullptr, nullptr);
	*logicGates	= xml::fetchObjectFromXML<utils::LogicGate>(doc, "//logic/gate", xml::extractGate, &validGates, flags, nullptr);


	//Get macros (shorthands for collection of elementary objects like walls or displacements)
	macros::fetchMacroFromXML(doc, "//environment/cuboid", macros::extractCuboid, visplaneData, wallData, flags, textureNames);
	macros::fetchMacroFromXML(doc, "//environment/stairs", macros::extractStairs, visplaneData, wallData, flags, textureNames);
	xml::loadModels(doc, "//environment/model", displacementData, textureNames);


	stageData.name = stageName;
	stageData.filePath = filePath;
	xml::retrieveStageMetaData(doc, player);
	processTeleporterPartners(visplaneData);
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


	//Handle settings that can have multiple string inputs, which map to other values.
	setConfigFromStringOptionsMap("VIEW_RENDER_RESOLUTION_QUALITY", &resolutionMap, "LOW", &desiredRenderResolution);
	setConfigFromStringOptionsMap("META_DEBUG_MODE", &debugMap, "NONE");
	setConfigFromStringOptionsMap("VIEW_TEXTURE_QUALITY", &texMipMap, "LOW");
	setConfigFromStringOptionsMap("VIEW_SHADOW_QUALITY", &shadowQualityMap, "LOW");

	if (utils::configToBool("META_SHOW_CONSOLE")) {
		utils::showConsole();
	} else {
		utils::hideConsole();
	}
}

}