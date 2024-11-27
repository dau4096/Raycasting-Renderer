//world.frag
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec2 playerPosition;
uniform bool zoom;



layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;
	float dimmingStrength;

    float toRad;

	vec3 topColour;
	vec3 lowColour;

	int drawUV;
};


struct Wall {
	vec2 start;		// Wall start point
	vec2 end;		// Wall end point
	int textureID;	// Wall texture Index
	int valid;		// Wall; Valid or not?
};
layout(std140, binding=2) uniform wallUBO {
	Wall walls[128];
};



//Ray Struct.
struct Ray {
	vec2 position, direction, end;
};

Ray createRay(vec2 position, vec2 direction) {
	Ray ray;
	ray.position = position;
	ray.direction = direction;
    ray.end = ray.position + (ray.direction * maxRayDistance);
	return ray;
};



float determinant(vec2 vecA, vec2 vecB) {
    return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}

float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}



vec4 drawWallLine(float lineHeight, Wall wall, vec2 position, float depth, float multiplier, ivec2 fragPosition, ivec2 screenDimentions, vec4 fragColour) {
	float fragRelativePosition = abs(fragPosition.y - screenDimentions.y);


	int midPointY = screenDimentions.y/2;

	if (fragRelativePosition > midPointY) {
		discard; //Above the top of the wall.
	}


	float repeatInterval = 1.0f;

	//UV X coordinate
	vec2 wallDirection = wall.end - wall.start;
	vec2 wallPosition = position - wall.start;
	float wallLength = length(wallDirection);
	float projection = dot(wallPosition, normalize(wallDirection));
	float xUV = mod(projection / repeatInterval, 1.0f);
	if (xUV < 0.0f) xUV += 1.0f;

	//UV Y coordinate
	float yUV = (fragPosition.y - (midPointY - lineHeight / 2.0f)) / lineHeight;
	yUV = clamp(yUV, 0.0f, 1.0f);


	vec3 UV = vec3(xUV, yUV, wall.textureID);
	return vec4(texture(textureArray, UV).rgb, depth);
}



vec2 castRay(Ray ray, Wall wall) {
    vec2 xDiff = vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
    vec2 yDiff = vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);

    float divisor = determinant(xDiff, yDiff);
    if (abs(divisor) < 1e-7) {
        //Lines do not intersect
        return vec2(1e30);
    }

    vec2 dets = vec2(determinant(ray.position, ray.end), determinant(wall.start, wall.end));
    float xCoord = determinant(dets, xDiff) / divisor;
    float yCoord = determinant(dets, yDiff) / divisor;

    vec2 intersectPoint = vec2(xCoord, yCoord);

    // Check if the intersection is within the wall segment
    if (intersectPoint.x < min(wall.start.x, wall.end.x) || intersectPoint.x > max(wall.start.x, wall.end.x) ||
        intersectPoint.y < min(wall.start.y, wall.end.y) || intersectPoint.y > max(wall.start.y, wall.end.y)) {
        return vec2(1e30); // Intersection is outside the wall segment
    }

    vec2 intersectDirection = normalize(intersectPoint - ray.position);
    vec2 directionDifference = ray.direction - intersectDirection;

    if (abs(directionDifference.x) < 0.1 && abs(directionDifference.y) < 0.1) {
        //Wrong way, behind camera.
        return vec2(1e30);
    }

    return intersectPoint;  
}



void main() {
	ivec2 fragPosition = ivec2(gl_FragCoord.xy);
	ivec2 screenDimentions = imageSize(renderedFrame);
	vec4 fragColour = (fragPosition.y > (screenDimentions.y/2)) ? vec4(0.5294, 0.8078, 0.9216, maxRayDistance) : vec4(0.5000, 0.5000, 0.5000, maxRayDistance);
	//vec4 fragColour = (fragPosition.y > (screenDimentions.y/2)) ? vec4(topColour.xyz, maxRayDistance) : vec4(lowColour.xyz, maxRayDistance);


	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = (((fragPosition.x / screenDimentions.x) * 2.0f) - 1.0f) * rayAngle;


	float angle = angleClamp(playerViewAngle + 180 + rayOffset);


	vec2 dirVec = normalize(vec2(sin(angle * toRad), cos(angle * toRad)));
	Ray ray = createRay(playerPosition, dirVec);

	float lowestDistance = maxRayDistance;
	//Invalid wall colour that will get overridden.
	float savedMultiplier = 0.0;
	vec2 closeIntersectPoint;
	Wall closestWall;


	for (int index = 0; index < 128; index++) {
        Wall wall = walls[index];
		if (wall.valid == 0) {continue;}
		vec2 intersectPoint = castRay(ray, wall);

		if (intersectPoint == vec2(1e30)) {continue;}

		float intersectDistance = length(intersectPoint - ray.position);


		if (intersectDistance < lowestDistance) {
			lowestDistance = intersectDistance;

			vec2 wallVec = normalize(wall.start - wall.end);
			float angleMultiplier = dot(wallVec, vec2(0, 1))* 0.2 + 0.8;
			float distanceMultiplier = 1.0f - (2.0f * intersectDistance) / maxRayDistance;
			float multiplier = angleMultiplier * distanceMultiplier;
			savedMultiplier = multiplier;
			closeIntersectPoint = intersectPoint;
			closestWall = wall;
		}
	}

	if (savedMultiplier != 0.0) {
		float correctionFactor = 0.25f; //Multiplies by amount of correction.
		float adjustedDistance = (1.0f - correctionFactor) * lowestDistance + correctionFactor * (lowestDistance * cos(rayOffset * toRad));
		float wallHeight = (screenDimentions.y / (adjustedDistance + 0.0001f)) * (maxRayAngle/rayAngle);


		fragColour = drawWallLine(wallHeight, closestWall, closeIntersectPoint, lowestDistance, savedMultiplier, fragPosition, screenDimentions, fragColour);
		
		//Only save in this case.
		//imageStore(renderedFrame, fragPosition, fragColour);
	}




	//Effectively CTRL+C/CTRL+V raycasting.cpp into this place, its all C syntax.
	//Just use `1.0f - ((fragPosition.x / screenDimentions.x) * 2.0f)` rather than a for-loop for xCoord values.
	//Check Y pixel value against fragPosition.y before drawing colour.
	//Use texture(...) and a sampler2DArray (ID is z) to get pixColour.
	//Use alpha-value as `depth/maxRayDistance`, to be utilised in sprites.frag and subsequently ignored in display.frag.

}