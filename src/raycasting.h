#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "includes.h"
#include "utils.h"
#include <array>

namespace raycasting {

	void checkRays(utils::FrameBuffer* frameBuffer,	utils::Player player, const std::array<utils::Wall, 128>* lineData,	std::array<unsigned char*, 16> textureArray, std::array<int, 16> textureChannels);
	glm::vec2 castRay(utils::Ray ray, utils::Wall wall);

}

#endif