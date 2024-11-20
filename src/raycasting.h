#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "includes.h"
#include "utils.h"

namespace raycasting {

	void checkRays(utils::FrameBuffer* frameBuffer, utils::Player player, const std::array<utils::Wall, 128>* lineData, unsigned char* textureData);
	glm::vec2 castRay(utils::Ray ray, utils::Wall wall);

}

#endif