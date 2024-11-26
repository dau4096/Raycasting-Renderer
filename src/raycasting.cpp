#include "includes.h"
#include "utils.h"
#include <array>
using namespace std;
using namespace utils;

/*
--Notes;
Send out a ray for every pixel collumn. (360px)
Ray angle at (for px=pixel) maxOffset * (px.X - 180)/180;
I.e px.X = 0, angle = -maxOffset; px.X = 180, angle = 0; px.X = 360, angle = maxOffset;
Get intersection of 2 infinite lines.
Check distance between intersection point and max distance from MidPoint of Wall
If valid, get distance from Ray.position to intersection point.
Draw onscreen.


--Sources;
https://rootllama.wordpress.com/2014/06/20/ray-line-segment-intersection-test-in-2d/
https://math.stackexchange.com/questions/2460414/how-to-determine-if-a-ray-intersects-a-line
https://stackoverflow.com/questions/14307158/how-do-you-check-for-intersection-between-a-line-segment-and-a-line-ray-emanatin
*/


namespace raycasting {


glm::vec2 castRay(utils::Ray ray, utils::Wall wall) {
	glm::vec2 xDiff = glm::vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	glm::vec2 yDiff = glm::vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);

	float divisor = utils::determinant(xDiff, yDiff);
	if (abs(divisor) < 1e-7) {
		//Lines do not intersect
		return glm::vec2(1e30);
	}

	glm::vec2 dets = glm::vec2(utils::determinant(ray.position, ray.end), utils::determinant(wall.start, wall.end));
	float xCoord = utils::determinant(dets, xDiff) / divisor;
	float yCoord = utils::determinant(dets, yDiff) / divisor;

	glm::vec2 intersectPoint = glm::vec2(xCoord, yCoord);

	// Check if the intersection is within the wall segment
	if (intersectPoint.x < std::min(wall.start.x, wall.end.x) || intersectPoint.x > std::max(wall.start.x, wall.end.x) ||
		intersectPoint.y < std::min(wall.start.y, wall.end.y) || intersectPoint.y > std::max(wall.start.y, wall.end.y)) {
		return glm::vec2(1e30); // Intersection is outside the wall segment
	}

	glm::vec2 intersectDirection = glm::normalize(intersectPoint - ray.position);
	glm::vec2 directionDifference = ray.direction - intersectDirection;

	if (abs(directionDifference.x) < 0.1 && abs(directionDifference.y) < 0.1) {
		//Wrong way, behind camera.
		return glm::vec2(1e30);
	}

	return intersectPoint;	
}


void checkRays(utils::FrameBuffer* frameBuffer, utils::Player player, const std::array<utils::Wall, 128>* wallData, std::array<utils::Texture, 16> textureArray, float rayAngle) {
	for (int xCoord = 0; xCoord < display::screenWidth; xCoord++) {
		float rayOffset = -rayAngle + (xCoord / (float)display::screenWidth) * 2 * rayAngle;
		float angle = utils::angleClamp(player.viewAngle + 180 + rayOffset);


		glm::vec2 dirVec = glm::normalize(glm::vec2(sin(angle * constants::toRad), cos(angle * constants::toRad)));
		utils::Ray ray = Ray(player.position, dirVec);
		ray.end = ray.position + (ray.direction * display::maxRayDistance);

		float lowestDistance = display::maxRayDistance;
		//Invalid wall colour that will get overridden.
		float savedMultiplier = 0.0;
		glm::vec2 closeIntersectPoint;
		utils::Wall closestWall;


		for (const utils::Wall& wall : *wallData) {
			if (!wall.valid) {continue;}
			glm::vec2 intersectPoint = raycasting::castRay(ray, wall);

			if (intersectPoint == glm::vec2(1e30)) {continue;}

			float intersectDistance = glm::length(intersectPoint - ray.position);


			if (intersectDistance < lowestDistance) {
				lowestDistance = intersectDistance;

				glm::vec2 wallVec = glm::normalize(wall.start - wall.end);
				float angleMultiplier = glm::dot(wallVec, glm::vec2(0, 1))* 0.2 + 0.8;
				float distanceMultiplier = 1.0f - (2.0f * intersectDistance) / display::maxRayDistance;
				float multiplier = angleMultiplier * distanceMultiplier;
				savedMultiplier = multiplier;
				closeIntersectPoint = intersectPoint;
				closestWall = wall;
			}
		}

		if (savedMultiplier != 0.0) {
			float correctionFactor = 0.25f; //Multiplies by amount of correction.
			float adjustedDistance = (1.0f - correctionFactor) * lowestDistance + correctionFactor * (lowestDistance * cos(rayOffset * constants::toRad));
			float wallHeight = (display::screenHeight / (adjustedDistance + 0.0001f)) * (display::maxRayAngle/rayAngle);

			utils::Texture texture = textureArray[closestWall.textureID];

			frameBuffer->drawWallLine(xCoord, wallHeight, closestWall, closeIntersectPoint, lowestDistance, texture, savedMultiplier);
		}
	}
}



int getSpriteScreenX(utils::Sprite sprite, utils::Player player, float onScreenWidth, bool zoom) {
	//Make sure to return a VERY offscreen x coordinate to be interpreted as "Invalid"
	glm::vec2 spriteDirection = glm::normalize(sprite.position - player.position);
	glm::vec2 playerDirection = glm::normalize(glm::vec2(sin(player.viewAngle * constants::toRad), cos(player.viewAngle * constants::toRad)));

	float dot = glm::dot(spriteDirection, playerDirection);
	float angleBetween = acos(glm::clamp(dot, -1.0f, 1.0f)) * constants::toDeg;
	if (angleBetween > 180.0f) { return -1e3; }

	float dotDegrees = (1.0f - dot) * 180.0f;

	float cross = spriteDirection.x * playerDirection.y - spriteDirection.y * playerDirection.x; // 2D cross product
	int dotDirection = (cross >= 0) ? 1 : -1;


	float screenXRelative = tan(angleBetween * constants::toRad) / tan(display::maxRayAngle * constants::toRad);
	screenXRelative = (zoom) ? screenXRelative*display::zoomFactor : screenXRelative;
	int centrePixelX = (display::screenWidth / 2) + (dotDirection * screenXRelative * (display::screenWidth / 2));

	if (centrePixelX + onScreenWidth/2 < 0 || centrePixelX - onScreenWidth/2 > display::screenWidth) {return -1e3;} //Also offscreen.

	return centrePixelX;
}



void drawSprites(utils::FrameBuffer* frameBuffer, utils::Player player, const std::array<utils::Sprite, 128>* spriteData, std::array<utils::Texture, 16> textureArray, bool zoom) {
	for (const utils::Sprite& sprite : *spriteData) {
		if (!sprite.valid) {continue;}
		float spriteDistance = glm::length(player.position - sprite.position);

		if (spriteDistance > display::maxRayDistance) {continue;} //Too far to see onscreen.

		float correctionFactor = 0.25f; //Multiplies by amount of correction.
		float adjustedDistance = (1.0f - correctionFactor) * spriteDistance + correctionFactor * spriteDistance;
		float spriteHeight = display::screenHeight / (adjustedDistance + 0.0001f);
		float spriteWidth = (sprite.width / adjustedDistance) * (display::screenWidth / (2 * tan(display::maxRayAngle * constants::toRad)));

		spriteWidth = (zoom) ? spriteWidth * display::zoomFactor : spriteWidth;
		spriteHeight = (zoom) ? spriteHeight * display::zoomFactor : spriteHeight;


		int centrePixelX = getSpriteScreenX(sprite, player, spriteWidth, zoom);
		if (centrePixelX < -(spriteWidth/2) || centrePixelX >= display::screenWidth + (spriteWidth/2)) {continue;} //Offscreen, horizontally.

		utils::Texture texture = textureArray[sprite.textureID];

		for (int xCoord = -spriteWidth/2; xCoord < spriteWidth/2; xCoord++) {
			frameBuffer->drawSpriteLine(xCoord + centrePixelX, spriteHeight, sprite, xCoord + spriteWidth/2, spriteWidth, spriteDistance, texture);
		}		
	}
}

}