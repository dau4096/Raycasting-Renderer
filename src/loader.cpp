#include "includes.h"
#include "global.h"
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
	{"G_INVALID", 0}, 		{"W_INVALID", 0}, 		{"V_INVALID", 0},		{"IFN_INVALID", 0}, 	{"SPR_INVALID", 0}, 
	{"G_PASSTHROUGH", 1}, 	{"W_NORMAL", 1},	 	{"V_NORMAL", 1}, 		{"IFN_UTILITY", 1},		{"SPR_DECO", 1}, 
	{"G_AND", 2}, 			{"W_TRIGGER", 2},	 	{"V_TRIGGER", 2}, 		{"IFN_HITSCAN", 2},		{"SPR_LIGHT", 2}, 
	{"G_OR", 3}, 			{"W_MOVEV_FAST", 3},	{"V_MOVEV_FAST", 3}, 	{"IFN_PROJECTILE", 3},
	{"G_NOT", 4}, 			{"W_MOVEV_SLOW", 4},	{"V_MOVEV_SLOW", 4}, 	{"IFN_MELEE", 4},
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

int envTexIdx = 0, UITexIdx = 0;
static inline int assignTexture(std::string textureStr, std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames, int* curTexIdx=&envTexIdx) {
	auto begin = textureNames->begin(), end = textureNames->end();
	auto namePTR = std::find(std::begin(*textureNames), std::end(*textureNames), textureStr);

	int idx = -1;

	if (namePTR != end) {
		idx = std::distance(begin, namePTR);
	} else {
		if (*curTexIdx >= display::TEXTURE_ARRAY_MAX_LAYERS) {
			raise("Maximum texture layers reached. Cannot assign more.");
			return -1;
		}
		textureNames->at(*curTexIdx) = textureStr;
		idx = *curTexIdx;
		(*curTexIdx)++;
	}
	return idx;
}





template<typename T, std::size_t N>
std::array<T, N> fetchObjectFromXML(
		const pugi::xml_document& doc,
		const std::string& xpath,
		std::function<T(
			const pugi::xml_node&,
			std::array<int, constants::MAX_FLAGS>* flags,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
		)> extractor,
		std::array<int, constants::MAX_FLAGS>* flags=nullptr,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames=nullptr
	)
{
	std::array<T, N> result{};
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath.c_str());
	size_t count = std::min(static_cast<size_t>(nodeList.size()), N);
	
	for (size_t i = 0; i < count; ++i) {
		pugi::xml_node node = nodeList[i].node();
		result[i] = extractor(node, flags, envTextureNames);
	}
	return result;
}




static inline Visplane extractVisplane(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
	) {
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string flagPTR = node.attribute("IOPtr").as_string();
	std::string textureStr = node.attribute("texture").as_string();
	Visplane visplane = Visplane(
		parseVec2(node.attribute("start").as_string()),
		parseVec2(node.attribute("end").as_string()),
		node.attribute("height").as_float(),
		assignTexture(textureStr, envTextureNames),
		static_cast<VisplaneType>(assignEnum(typeStr)),
		managePTR(flagPTR, flags),
		node.attribute("extra").as_float()
	);
	
	return visplane;
}


static inline Wall extractWall(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
	) {
	glm::vec3 start = parseVec3(node.attribute("start").as_string());
	glm::vec3 end = parseVec3(node.attribute("end").as_string());
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string flagPTR = node.attribute("IOPtr").as_string();
	std::string textureStr = node.attribute("texture").as_string();
	Wall wall = Wall(
		parseVec3(node.attribute("start").as_string()), 
		parseVec3(node.attribute("end").as_string()),
		assignTexture(textureStr, envTextureNames),
		static_cast<WallType>(assignEnum(typeStr)),
		managePTR(flagPTR, flags),
		node.attribute("extra").as_float()
	);
	
	return wall;
}


static inline Sprite extractSprite(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
	) {
	std::string typeStr = strToUpper(node.attribute("type").as_string());
	std::string textureStr = node.attribute("texture").as_string();
	Sprite sprite = Sprite(
		parseVec3(node.attribute("position").as_string()),
		node.attribute("width").as_float(),
		node.attribute("height").as_float(),
		assignTexture(textureStr, envTextureNames),
		static_cast<SpriteType>(assignEnum(typeStr)),
		strToUpper(node.attribute("collision").as_string()) == "TRUE"
	);
	
	return sprite;
}


static inline Light extractLight(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
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


static inline TextObject extractTextObject(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
	) {
	std::string flagPTR = node.attribute("inputPTR").as_string();
	TextObject textObject = TextObject(
		node.attribute("text").as_string(),
		parseVec3(node.attribute("position").as_string()),
		node.attribute("scale").as_int()
	);
	
	return textObject;
}


static inline LogicGate extractGate(
		const pugi::xml_node& node,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
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


static inline std::array<std::string, playerConfig::MAX_ITEMS_HELD> unpackItems(std::string itemsStr) {
	std::array<std::string, playerConfig::MAX_ITEMS_HELD> result;
	std::stringstream itemsStrUpper(utils::strToUpper(itemsStr));
	std::string segment;
	char seperator = ';';
	int idx = 0;

	while (std::getline(itemsStrUpper, segment, seperator) && (idx < playerConfig::MAX_ITEMS_HELD)) {
		result[idx] = segment;
		idx++;
	}

	return result;
}


void retrieveStageMetaData(const pugi::xml_document& doc, utils::Player* player) {
	//Sky
	pugi::xml_node skyNode = getMetaNode(doc, "sky");
	stageData.skyboxTextureName = skyNode.attribute("skyboxTexture").as_string();


	//Sun
	pugi::xml_node sunNode = getMetaNode(doc, "sun");
	stageData.sunDirection = parseVec3(sunNode.attribute("sunDirection").as_string());
	float sunIntensity = sunNode.attribute("sunIntensity").as_float();
	stageData.sunColour = parseVec3(sunNode.attribute("sunColour").as_string()) * sunIntensity;


	//Physics
	pugi::xml_node physNode = getMetaNode(doc, "physics");
	stageData.gravity = physNode.attribute("gravity").as_float();


	//Player
	pugi::xml_node playerNode = getMetaNode(doc, "player");
	stageData.playerStartPoint = parseVec3(playerNode.attribute("startPoint").as_string());
	stageData.playerStartAngle = playerNode.attribute("startAngle").as_float();

	std::string startHealthStr = playerNode.attribute("initialHealth").as_string();
	stageData.playerStartHealth = handlePlayerHEString(startHealthStr, playerConfig::PLAYER_MAX_HEALTH);

	std::string startEnergyStr = playerNode.attribute("initialEnergy").as_string();
	stageData.playerStartEnergy = handlePlayerHEString(startEnergyStr, playerConfig::PLAYER_MAX_ENERGY);

	std::string initialPlayerItemsStr = playerNode.attribute("items").as_string();
	stageData.initialPlayerItems = unpackItems(initialPlayerItemsStr);

	*player = utils::Player();

	player->heldItemIdx = -1;
	player->heldItemPTR = &(itemData[playerConfig::EMPTY_HAND_ITEM_NAME]);
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



static inline int attrBoolInt(std::string input) {
	std::string upper = utils::strToUpper(input);
	if ((upper == "TRUE") || (upper == "T")) {
		return 1;
	} else if ((upper == "FALSE") || (upper == "F")) {
		return 0;
	} else {
		raise("Unknown boolean attribute: " + input);
	}
	return -1;
}

void fetchItemsFromXML(const pugi::xml_document& doc, std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* UIImageNames) {
	const char* xpath = "//items/item";
	pugi::xpath_node_set nodeList = doc.select_nodes(xpath);
	size_t count = static_cast<size_t>(nodeList.size());

	int initialLen = 0;
	for (std::string imgName : *UIImageNames) {
		if (imgName == "") {
			break;
		}
		initialLen++;
	}
	UITexIdx = initialLen;

	
	std::string functionString, valueString;
	for (size_t i = 0; i < count; ++i) {
		pugi::xml_node node = nodeList[i].node();

		std::array<float, 5> data;
		std::string typeStr = node.attribute("type").as_string();
		ItemFunction type = static_cast<ItemFunction>(assignEnum(typeStr));
		switch (type) {
		case IFN_UTILITY: {
			data[0] = attrBoolInt(node.attribute("toggle").as_string());
			data[1] = node.attribute("maxUses").as_int();
			data[2] = node.attribute("energyUse").as_int();
			data[3] = node.attribute("healthGiven").as_int();
			data[4] = attrBoolInt(node.attribute("envIlluminate").as_string());
			break;
		}

		case IFN_HITSCAN: {
			data[0] = node.attribute("usePerSecond").as_float();
			data[1] = node.attribute("energyUse").as_int();
			data[2] = node.attribute("strength").as_int();
			data[3] = node.attribute("raysPerUse").as_int();
			data[4] = node.attribute("raySpread").as_float();
			break;
		}

		case IFN_PROJECTILE: {
			data[0] = node.attribute("usePerSecond").as_float();
			data[1] = node.attribute("maxUses").as_int();
			data[2] = node.attribute("energyUse").as_int();
			data[3] = node.attribute("strength").as_int();
			data[4] = node.attribute("projectileSpeed").as_float();
			break;
		}

		case IFN_MELEE: {
			data[0] = node.attribute("usePerSecond").as_float();
			data[1] = node.attribute("maxUses").as_int();
			data[2] = node.attribute("energyUse").as_int();
			data[3] = node.attribute("strength").as_int();
			break;
		}

		default: {
			raise("Unknown item type: " + typeStr);
			break;
		}
		}


		//Textures
		std::array<int, 5> textureIDs;
		std::string textureStr;
		//IS_IDLE
		textureStr = node.attribute("texture-idle").as_string();
		textureIDs[0] = assignTexture(textureStr, UIImageNames, &UITexIdx);
		//IS_ACTIVE
		textureStr = node.attribute("texture-active").as_string();
		textureIDs[1] = assignTexture(textureStr, UIImageNames, &UITexIdx);
		//IS_RECHARGE
		textureStr = node.attribute("texture-recharge").as_string();
		textureIDs[2] = assignTexture(textureStr, UIImageNames, &UITexIdx);
		//IS_DEPLETED
		textureStr = node.attribute("texture-depleted").as_string();
		textureIDs[3] = assignTexture(textureStr, UIImageNames, &UITexIdx);
		//IS_INVALID
		textureIDs[4] = -1;

		std::string itemName = utils::strToUpper(node.attribute("name").as_string());
		Item thisItem = Item(
			itemName, type,
			data, textureIDs
		);


		itemData[itemName] = thisItem;
	}



	//Add empty item (nothing held)
	std::array<float, 5> emptyfData;
	std::array<int, 5> emptyiData = {-1, -1, -1, -1, -1};
	Item emptyItem = Item(
		playerConfig::EMPTY_HAND_ITEM_NAME, IFN_EMPTY,
		emptyfData, emptyiData
	);
	itemData[playerConfig::EMPTY_HAND_ITEM_NAME] = emptyItem;
}



namespace loader {


void loadStage(
		const std::string& stageName, utils::Player* player,
		std::array<utils::Visplane, constants::MAX_VISPLANES>* visplaneData,
		std::array<utils::Wall, constants::MAX_WALLS>* wallData,
		std::array<utils::Sprite, constants::MAX_SPRITES>* spriteData,
		std::array<utils::Light, constants::MAX_LIGHTS>* lightData,
		std::array<utils::TextObject, constants::MAX_TEXT_OBJECTS>* textObjectData,
		std::array<utils::LogicGate, constants::MAX_GATES>* logicGates,
		std::array<int, constants::MAX_FLAGS>* flags,
		std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* envTextureNames
	) {
	std::string filePath = "stages/" + stageName + ".xml";
	std::string XMLSrc = utils::readFile(filePath);


	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}
	
	*visplaneData = fetchObjectFromXML<utils::Visplane, constants::MAX_VISPLANES>(doc, "//visplanes/visplane", extractVisplane, flags, envTextureNames);
	*wallData = fetchObjectFromXML<utils::Wall, constants::MAX_WALLS>(doc, "//walls/wall", extractWall, flags, envTextureNames);
	*spriteData	= fetchObjectFromXML<utils::Sprite, constants::MAX_SPRITES>(doc, "//sprites/sprite", extractSprite, nullptr, envTextureNames);
	*lightData = fetchObjectFromXML<utils::Light, constants::MAX_LIGHTS>(doc, "//lights/light", extractLight, nullptr, nullptr);
	*textObjectData = fetchObjectFromXML<utils::TextObject, constants::MAX_TEXT_OBJECTS>(doc, "//objects/textObj", extractTextObject, nullptr, nullptr);
	*logicGates	= fetchObjectFromXML<utils::LogicGate, constants::MAX_GATES>(doc, "//logicGates/logic", extractGate, flags, nullptr);


	retrieveStageMetaData(doc, player);
}


void loadBindings() {
	const std::string filePath = "xml/userConfig.xml";
	std::string XMLSrc = utils::readFile(filePath);

	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}

	fetchBindingsFromXML(doc);
	fetchConfigsFromXML(doc);


	if (utils::configToBool("META_SHOW_CONSOLE")) {
		utils::showConsole();
	} else {
		utils::hideConsole();
	}
}


void loadGameData(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* UIImageNames) {
	const std::string filePath = "xml/gameData.xml";
	std::string XMLSrc = utils::readFile(filePath);

	pugi::xml_document doc;
	pugi::xml_parse_result parseResult = doc.load_string(XMLSrc.c_str());
	if (!parseResult) {
		throw std::runtime_error("Failed to parse XML: " + std::string(parseResult.description()));
	}


	fetchItemsFromXML(doc, UIImageNames);
}

}