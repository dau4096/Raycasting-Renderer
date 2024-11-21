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


void checkRays(utils::FrameBuffer* frameBuffer, utils::Player player, const std::array<utils::Wall, 128>* wallData, std::array<unsigned char*, 16> textureArray, std::array<int, 16> textureChannels) {
	for (int xCoord = 0; xCoord < display::screenWidth; xCoord++) {
		float rayOffset = -display::maxRayAngle + (xCoord / (float)display::screenWidth) * 2 * display::maxRayAngle;
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
			float wallHeight = display::screenHeight / (adjustedDistance + 0.0001f);

			unsigned char* textureData = textureArray[closestWall.textureID];
			int channels = textureChannels[closestWall.textureID];

			frameBuffer->drawLine(xCoord, wallHeight, closestWall, closeIntersectPoint, lowestDistance, textureData, channels, savedMultiplier);
		}
	}
}

}