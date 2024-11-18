#include "includes.h"
#include "utils.h"
using namespace std;
using namespace global;
using namespace glm;

namespace global {

	vec2 playerPosition(0.0f, 0.0f);
	float playerViewAngle = 0.0f;

	unordered_map<int, bool> keyMap = {};

	//Warning; Compiler error, can't be bothered to fix it currently.
	//utils::Line levelData[256];

}