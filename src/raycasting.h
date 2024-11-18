#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "includes.h"
#include "utils.h"

namespace raycasting {

	void checkRays(utils::FrameBuffer* frameBuffer, float playerViewAngle, glm::vec2 playerPosition, const std::array<utils::Wall, 128>* lineData);

}

#endif