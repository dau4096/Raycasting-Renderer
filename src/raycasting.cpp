#include "includes.h"
#include "utils.h"
#include "physics.h"
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

void checkRays(utils::FrameBuffer* frameBuffer, float playerViewAngle, glm::vec2 playerPosition, const std::array<utils::Wall, 128>* wallData) {
	for (int xCoord = 0; xCoord < display::screenWidth; xCoord++) {
		float rayOffset = -display::maxRayAngle + (xCoord / (float)display::screenWidth) * 2 * display::maxRayAngle;
		float angle = physics::angleClamp(playerViewAngle + 180 + rayOffset);


		glm::vec2 dirVec = glm::normalize(glm::vec2(sin(angle * constants::toRad), cos(angle * constants::toRad)));
		utils::Ray ray = Ray(playerPosition, dirVec);
		ray.end = ray.position + (ray.direction * display::maxRayDistance);

		float lowestDistance = display::maxRayDistance;
		//Invalid wall colour that will get overridden.
		glm::vec3 wallColour = glm::vec3(2.0);


		for (const utils::Wall& wall : *wallData) {
			glm::vec2 xDiff = glm::vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
			glm::vec2 yDiff = glm::vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);

			float divisor = utils::determinant(xDiff, yDiff);
			if (abs(divisor) < 1e-7) {
				//Lines do not intersect
				continue;
			}

			glm::vec2 dets = glm::vec2(utils::determinant(ray.position, ray.end), utils::determinant(wall.start, wall.end));
			float xCoord = utils::determinant(dets, xDiff) / divisor;
			float yCoord = utils::determinant(dets, yDiff) / divisor;

			glm::vec2 intersectPoint = glm::vec2(xCoord, yCoord);

			// Check if the intersection is within the wall segment
			if (intersectPoint.x < std::min(wall.start.x, wall.end.x) || intersectPoint.x > std::max(wall.start.x, wall.end.x) ||
				intersectPoint.y < std::min(wall.start.y, wall.end.y) || intersectPoint.y > std::max(wall.start.y, wall.end.y)) {
				continue; // Intersection is outside the wall segment
			}

			glm::vec2 intersectDirection = glm::normalize(intersectPoint - ray.position);
			glm::vec2 directionDifference = ray.direction - intersectDirection;

			if (abs(directionDifference.x) < 0.1 && abs(directionDifference.y) < 0.1) {
				//Wrong way, behind camera.
				continue;
			}

			float intersectDistance = glm::length(intersectPoint - ray.position);


			if (intersectDistance < lowestDistance) {
				lowestDistance = intersectDistance;
				wallColour = wall.colour;
			}
		}

		if (wallColour != glm::vec3(2.0)) {
			float wallHeight = display::screenHeight / (lowestDistance + 0.0001f); // Small offset to avoid division by zero
			wallHeight = wallHeight / (cos(rayOffset * constants::toRad) + 1.0f);

			frameBuffer->drawLine(xCoord, wallHeight, wallColour);
		}
	}
}


}