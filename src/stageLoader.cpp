#include "includes.h"
#include "utils.h"
using namespace std;
using namespace utils;
using namespace glm;
using namespace pugi;


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
		return &(constants::TRUE);
	} else if ((ptrStrUpper == "FALSE") || (ptrStrUpper == "NEVER")) {
		return &(constants::FALSE);
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
	return &(constants::FALSE);
};



static const std::unordered_map<std::string, int> enumMap = {
	{"G_INVALID", 0}, 		{"W_INVALID", 0}, 		{"V_INVALID", 0}, 		{"SPR_INVALID", 0}, 
	{"G_PASSTHROUGH", 1}, 	{"W_NORMAL", 1},	 	{"V_NORMAL", 1}, 		{"SPR_DECO", 1}, 
	{"G_AND", 2}, 			{"W_TRIGGER", 2},	 	{"V_TRIGGER", 2}, 		{"SPR_LIGHT", 2}, 
	{"G_OR", 3}, 			{"W_MOVEV_FAST", 3},	{"V_MOVEV_FAST", 3}, 
	{"G_NOT", 4}, 			{"W_MOVEV_SLOW", 4},	{"V_MOVEV_SLOW", 4}, 
	{"G_XOR", 5}, 			{"W_MOVEH_FAST", 5},	{"V_HURT", 5}, 
	{"G_LATCH", 6}, 		{"W_MOVEH_SLOW", 6},
	{"G_PULSE", 7}, 		{"W_SWITCH", 7}, 
	{"G_TOGGLE", 8}
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
	auto namePTR = std::find(std::begin(*textureNames), std::end(*textureNames), textureStr);

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





template<typename T, std::size_t N>
std::array<T, N> fetchFromXML(
		const pugi::xml_document& doc,
		const std::string& xpath,
		std::function<T(
			const pugi::xml_node&,
			std::array<int, constants::MAX_FLAGS>* flags,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
		)> extractor,
		std::array<int, constants::MAX_FLAGS>* flags=nullptr,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames=nullptr
	)
{
	std::array<T, N> result{};
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = std::min(static_cast<size_t>(nodeList.size()), N);
	
	for (size_t i = 0; i < count; ++i) {
		pugi::xml_node node = nodeList[i].node();
		result[i] = extractor(node, flags, textureNames);
	}
	return result;
}




static inline Visplane extractVisplane(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string flagPTR = node.attribute("IOPtr").as_string();
	std::string textureStr = node.attribute("texture").as_string();
	Visplane visplane = Visplane(
		parseVec2(node.attribute("start").as_string()),
		parseVec2(node.attribute("end").as_string()),
		node.attribute("height").as_float(),
		assignTexture(textureStr, textureNames),
		static_cast<VisplaneType>(assignEnum(typeStr)),
		managePTR(flagPTR, flags),
		node.attribute("extra").as_float()
	);
	
	return visplane;
}


static inline Wall extractWall(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	glm::vec3 start = parseVec3(node.attribute("start").as_string());
	glm::vec3 end = parseVec3(node.attribute("end").as_string());
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string flagPTR = node.attribute("IOPtr").as_string();
	std::string textureStr = node.attribute("texture").as_string();
	Wall wall = Wall(
		parseVec3(node.attribute("start").as_string()), 
		parseVec3(node.attribute("end").as_string()),
		assignTexture(textureStr, textureNames),
		static_cast<WallType>(assignEnum(typeStr)),
		managePTR(flagPTR, flags),
		node.attribute("extra").as_float()
	);
	
	return wall;
}


static inline Sprite extractSprite(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string textureStr = node.attribute("texture").as_string();
	Sprite sprite = Sprite(
		parseVec3(node.attribute("position").as_string()),
		node.attribute("width").as_float(),
		node.attribute("height").as_float(),
		assignTexture(textureStr, textureNames),
		static_cast<SpriteType>(assignEnum(typeStr)),
		strToUpper(node.attribute("collision").as_string()) == "TRUE"
	);
	
	return sprite;
}


static inline Light extractLight(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string flagPTR = node.attribute("inputPTR").as_string();
	Light light = Light(
		parseVec3(node.attribute("position").as_string()),
		parseVec3(node.attribute("colour").as_string()),
		node.attribute("intensity").as_int(),
		managePTR(flagPTR, flags)
	);
	
	return light;
}


static inline LogicGate extractGate(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames
	) {
	std::string typeStr = node.attribute("type").as_string();
	LogicGate gate = LogicGate(
		static_cast<GateType>(assignEnum(typeStr)),
		managePTR(node.attribute("outputPtr").as_string(), flags),
		managePTR(node.attribute("inputAPtr").as_string(), flags),
		managePTR(node.attribute("inputBPtr").as_string(), flags)
	);
	return gate;
}



namespace stageLoader {


void loadStage(
		const std::string& stageName,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData,
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Light, constants::MAX_LIGHTS>* lightData,
		std::array<utils::LogicGate, constants::MAX_GATES>* logicGates,
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
	
	// Now use the generic fetch function to extract each element type.
	*visplaneData = fetchFromXML<utils::Visplane, constants::MAX_VISPLANES>(doc, "//visplanes/visplane", extractVisplane, flags, textureNames);
	*wallData = fetchFromXML<utils::Wall, constants::MAX_WALLS>(doc, "//walls/wall", extractWall, flags, textureNames);
	*spriteData	= fetchFromXML<utils::Sprite, constants::MAX_SPRITES>(doc, "//sprites/sprite", extractSprite, nullptr, textureNames);
	*lightData = fetchFromXML<utils::Light, constants::MAX_LIGHTS>(doc, "//lights/light", extractLight, nullptr, nullptr);
	*logicGates	= fetchFromXML<utils::LogicGate, constants::MAX_GATES>(doc, "//logicGates/logic", extractGate, flags, nullptr);
}

}