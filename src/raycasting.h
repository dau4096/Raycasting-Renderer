#ifndef RAYCASTING_H
#define RAYCASTING_H

#include "includes.h"
#include "utils.h"
#include <array>

namespace raycasting {
	
	glm::vec2 castRay(utils::Ray ray, utils::Wall wall);
	void checkRays(utils::FrameBuffer* frameBuffer,	utils::Player player, const std::array<utils::Wall, 128>* lineData,	std::array<utils::Texture, 16> textureArray, float rayAngle);
	void drawSprites(utils::FrameBuffer* frameBuffer, utils::Player player, const std::array<utils::Sprite, 128>* spriteData, std::array<utils::Texture, 16> textureArray, bool zoom);

}

#endif