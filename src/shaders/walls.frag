/* walls.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec2 playerPosition;
uniform bool zoom;
uniform int drawUV;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
    float zoomFactor;
    float maxRayAngle;
    float maxRayDistance;

    float topIndex;
    float lowIndex;

    vec2 textureSize;

    float padding[2];
};
layout(std430, binding = 5) buffer depthBuffer {
	float depths[];
};


struct Wall {
    vec2 start;			//Wall Start.
    vec2 end;			//Wall End.
    int textureID;		//Wall Texture.
    int valid;			//Wall Validity.
    float padding[2];	//Wall Padding.
};

layout(std140, binding = 2) uniform wallUBO {
	Wall walls[256];
};


struct Light {
	vec3 position;		//Light Position
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float padding[3];	//Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[32];
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


vec2 fragPosition;
ivec2 renderResolution;
vec4 fragColour;


float determinant(vec2 vecA, vec2 vecB) {
	return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}



float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}


vec2 rayIntersectCheck(Ray ray, Wall wall) {
	vec2 xDiff = vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	vec2 yDiff = vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);


	double divisor = determinant(xDiff, yDiff);
	//If less than some Epsilon value.
	if (abs(divisor) < 1e-5f) {
		//Lines do not intersect, as they are nearly parrallel.
		return vec2(1e30f, 1e30f);
	}


	vec2 dets = vec2(determinant(ray.position, ray.end), determinant(wall.start, wall.end));
	double xCoord = determinant(dets, xDiff) / divisor;
	double yCoord = determinant(dets, yDiff) / divisor;

	vec2 intersectPoint = vec2(xCoord, yCoord);


	//Check if the intersection is within the wall segment.
	if (intersectPoint.x < min(wall.start.x, wall.end.x) || intersectPoint.x > max(wall.start.x, wall.end.x) ||
		intersectPoint.y < min(wall.start.y, wall.end.y) || intersectPoint.y > max(wall.start.y, wall.end.y)) {
		return vec2(1e30f, 1e30f); // Intersection is outside the wall segment
	}


	vec2 intersectDirection = normalize(intersectPoint - ray.position);
	vec2 directionDifference = ray.direction - intersectDirection;

	
	if (length(directionDifference) < 0.1f) {
		//Wrong way, behind camera.
		return vec2(1e30f, 1e30f);
	}
	

	return intersectPoint;  
}


vec2 getWallUV(Wall thisWall, vec2 intersectPoint, float wallHeight) {
	float textureRepeatInterval = 1.0f;

	//xUV calculation.
	vec2 wallDirection = thisWall.end - thisWall.start;
	vec2 wallPosition = intersectPoint - thisWall.start;
	float wallLength = length(wallDirection);
	float projection = dot(wallPosition, normalize(wallDirection));
	float xUV = fract((projection) / textureRepeatInterval);


	//yUV calculation.
	int midPointY = renderResolution.y / 2;
	float yCoordScreen = midPointY + fragPosition.y;
	float wallTop = midPointY - wallHeight / 2.0f;
	float wallBottom = midPointY + wallHeight / 2.0f;


	//Don't allow drawing above/below wall top/bottom respectively.
	if (fragPosition.y < wallTop || fragPosition.y > wallBottom) {return vec2(1e30f, 1e30f); /* Outside of wall bounds. */}
	float yUV = (fragPosition.y - (midPointY - wallHeight / 2.0f)) / wallHeight;


	return vec2(xUV, 1.0f - yUV);
}


vec4 getWallColour(float rayAngle, Wall closestWall, vec2 closestIntersectPoint, float minDistance) {
	vec4 INVALID = vec4(1e30f, 1e30f, 1e30f, 1e30f);
	if (minDistance == maxRayDistance) {return INVALID; /* No collision was detected. */}


	float wallMaxHeight = 1.0f;

	float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));
	float viewAngleOffset = atan(wallMaxHeight/minDistance);
	float wallHeight = (renderResolution.y * 2 * viewAngleOffset) / verticalFOV;

	
	vec2 wallVec = normalize(closestWall.start - closestWall.end);
	float angleMultiplier = dot(wallVec, vec2(0.0f, 1.0f)) * 0.2f + 0.8f;
	float distanceMultiplier = 1.0f - (minDistance / maxRayDistance);
	float multiplier = angleMultiplier * distanceMultiplier;

	vec2 wallUV = getWallUV(closestWall, closestIntersectPoint, wallHeight);
	if (wallUV == INVALID.xy) {return INVALID; /* Invalid UV coordinates. */}

	vec4 thisFragColour;
	if (drawUV == 1) {
		thisFragColour = vec4(wallUV.xy, closestWall.textureID / 32.0f, maxRayDistance);
	} else {
		thisFragColour = texture(textureArray, vec3(wallUV.xy, float(closestWall.textureID))) * multiplier;
	}

	return thisFragColour;
}


void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = imageLoad(renderedFrame, framePosition);


	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;
	float angle = radians(angleClamp(playerViewAngle + 180.0f + rayOffset));

	vec2 rayDirection = vec2(sin(angle), cos(angle));
	Ray fragRay = createRay(playerPosition, rayDirection);


	float minDistance = maxRayDistance, secondMinDistance = maxRayDistance;
	Wall closestWall, secondClosestWall;
	vec2 closestIntersectPoint, secondClosestIntersectPoint;

	//Iterate through all the walls.
	int i=0;
	for (int index = 0; index < 256; index++) {
		Wall thisWall = walls[index];
		thisWall.padding[0] = 0.0f; thisWall.padding[1] = 0.0f;
		if (thisWall.valid == 0) {continue; /* Wall is empty */}

		vec2 intersectPoint = rayIntersectCheck(fragRay, thisWall);
		if (intersectPoint == vec2(1e30f, 1e30f)) {continue; /* Invalid intersect point */}
		float wallDistance = length(playerPosition - intersectPoint);


		if (wallDistance >= minDistance) {continue;}
		
		//Set closest.
		secondMinDistance = minDistance;
		minDistance = wallDistance;
		secondClosestWall = closestWall;
		closestWall = thisWall;
		secondClosestIntersectPoint = closestIntersectPoint;
		closestIntersectPoint = intersectPoint;

		vec4 wallColour = getWallColour(rayAngle, closestWall, closestIntersectPoint, minDistance);
		if (wallColour != vec4(1e30f, 1e30f, 1e30f, 1e30f)) {
			fragColour = vec4(wallColour.a * wallColour.rgb, minDistance);
		}
	}







	//Save to texture.
	vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
	depths[framePosition.x] = minDistance;
	imageStore(renderedFrame, framePosition, finalFragColour);
}