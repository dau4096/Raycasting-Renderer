extern "C" {
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/display.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/keyboard.h"
	#include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/rtc.h"
}

#include "utils.h"
using namespace std;

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


vec2 castRay(Ray ray, Wall wall) {
	vec2 xDiff = vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	vec2 yDiff = vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);

	float divisor = utils::determinant(xDiff, yDiff);
	if (utils::abs(divisor) < 1e-7) {
		//Lines do not intersect
		return INVALID;
	}

	vec2 dets = vec2(utils::determinant(ray.position, ray.end), utils::determinant(wall.start, wall.end));
	float xCoord = utils::determinant(dets, xDiff) / divisor;
	float yCoord = utils::determinant(dets, yDiff) / divisor;

	vec2 intersectPoint = vec2(xCoord, yCoord);

	// Check if the intersection is within the wall segment
	if (intersectPoint.x < utils::min(wall.start.x, wall.end.x) || intersectPoint.x > utils::max(wall.start.x, wall.end.x) ||
		intersectPoint.y < utils::min(wall.start.y, wall.end.y) || intersectPoint.y > utils::max(wall.start.y, wall.end.y)) {
		return INVALID; //Intersection is outside the wall segment
	}

	vec2 intersectDirection = utils::normalise(utils::SUB(intersectPoint, ray.position));
	vec2 directionDifference = utils::SUB(ray.direction, intersectDirection);

	if (utils::abs(directionDifference.x) < 0.1 && utils::abs(directionDifference.y) < 0.1) {
		//Wrong way, behind camera.
		return INVALID;
	}

	return intersectPoint;	
}


void renderScene(Player player, const Wall (*wallData)[64], int maxWalls) {
	const float rayAngle = 35.0f;
	const float maxRayDistance = 64.0f;

	for (int xCoord = 0; xCoord < LCD_WIDTH_PX; xCoord++) {
		float rayOffset = -rayAngle + (xCoord / (float)LCD_WIDTH_PX) * 2 * rayAngle;
		float angle = utils::angleClamp(player.viewAngle + 180 + rayOffset); //Rough re-implemenation of fmod()


		vec2 dirVec = utils::normalise(vec2(utils::sin(angle), utils::cos(angle)));
		Ray ray = Ray(player.position, dirVec);
		ray.end = utils::ADD(ray.position, utils::MUL(ray.direction, maxRayDistance));

		float lowestDistance = maxRayDistance;
		//Invalid wall colour that will get overridden.
		float savedMultiplier = 0.0;
		vec2 closeIntersectPoint;
		Wall closestWall;


		for (int idx = 0; idx < maxWalls; idx++) {
			const Wall& thisWall = *wallData[idx];
			if (!thisWall.valid) {continue; /* Re-Check the wall is valid. */}
			vec2 intersectPoint = castRay(ray, thisWall);

			if (intersectPoint.x == INVALID.x && intersectPoint.y == INVALID.y) {continue;}

			float intersectDistance = utils::length(utils::SUB(intersectPoint, ray.position));


			if (intersectDistance < lowestDistance) {
				lowestDistance = intersectDistance;

				vec2 wallVec = utils::normalise(utils::SUB(thisWall.start, thisWall.end));
				float angleMultiplier = utils::dot(wallVec, vec2(0, 1))* 0.2 + 0.8; //Apply shading based off of angle.
				float distanceMultiplier = 1.0f - (2.0f * intersectDistance) / maxRayDistance;
				float multiplier = angleMultiplier * distanceMultiplier;
				savedMultiplier = multiplier;
				closeIntersectPoint = intersectPoint;
				closestWall = thisWall;
			}
		}

		if (savedMultiplier != 0.0) {
			float correctionFactor = 0.25f; //Multiplies by amount of correction.
			float adjustedDistance = (1.0f - correctionFactor) * lowestDistance + correctionFactor * (lowestDistance * utils::cos(rayOffset));
			float wallHeight = (LCD_HEIGHT_PX / (adjustedDistance + 0.0001f));

			vec3 baseColour(255, 0, 255);
			unsigned short finalColour = utils::createColour(baseColour.x * savedMultiplier, baseColour.y * savedMultiplier, baseColour.z * savedMultiplier);
			utils::drawLine(xCoord, wallHeight, finalColour);
		} else {
			utils::drawLine(xCoord, 0.0f, 0x0000); //No Wall.
		}
	}
}

}