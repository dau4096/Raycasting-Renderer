/* walls.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec3 playerPosition;
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
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[64];
};
layout(std430, binding = 5) buffer depthBuffer {
	float depths[];
};



//Ray Struct.
struct Ray {
	vec2 position, direction, end;
};

Ray createRay(vec2 position, vec2 direction, float maxDist=maxRayDistance) {
	Ray ray;
	ray.position = position;
	ray.direction = direction;
	ray.end = ray.position + (ray.direction * maxDist);
	return ray;
};


vec2 fragPosition;
ivec2 renderResolution;
vec4 fragColour;
float verticalFOV;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.1f;
const vec2 INVALID = vec2(1e30f, 1e30f);
const vec4 INVALIDv4 = vec4(1e30f, 1e30f, 1e30f, 1e30f);

const bool noLighting = true;


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
	if (abs(divisor) < EPSILON) {
		//Lines do not intersect, as they are nearly parrallel.
		return INVALID;
	}


	vec2 dets = vec2(determinant(ray.position, ray.end), determinant(wall.start, wall.end));
	double xCoord = determinant(dets, xDiff) / divisor;
	double yCoord = determinant(dets, yDiff) / divisor;

	vec2 intersectPoint = vec2(xCoord, yCoord);


	//Check if the intersection is within the wall segment.
	if (intersectPoint.x < min(wall.start.x, wall.end.x) || intersectPoint.x > max(wall.start.x, wall.end.x) ||
		intersectPoint.y < min(wall.start.y, wall.end.y) || intersectPoint.y > max(wall.start.y, wall.end.y)) {
		return INVALID; // Intersection is outside the wall segment
	}


	vec2 intersectDirection = normalize(intersectPoint - ray.position);
	vec2 directionDifference = ray.direction - intersectDirection;

	
	if (length(directionDifference) < EPSILON_ALT) {
		//Wrong way, behind camera.
		return INVALID;
	}
	

	return intersectPoint;  
}


vec2 getWallUV(Wall thisWall, vec2 intersectPoint, float wallLowZ, float wallTopZ) {
	const float textureRepeatInterval = 2.0f;

	//xUV calculation.
	vec2 wallDirection = thisWall.end - thisWall.start;
	vec2 wallPosition = intersectPoint - thisWall.start;
	float wallLength = length(wallDirection);
	float projection = dot(wallPosition, normalize(wallDirection));
	float xUV = fract(projection / textureRepeatInterval);


	//yUV calculation.
	float zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float distance = length(playerPosition.xy - intersectPoint) / zoomEffect;
	float projectedYLow = (playerPosition.z - wallLowZ) / distance;
	float projectedYTop = (playerPosition.z - wallTopZ) / distance;

	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	float screenYTop = renderResolution.y * (0.5 - projectedYTop);

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALID;}

	float a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow); //Alpha to mix by.
	float actualZ = mix(wallLowZ, wallTopZ, a);
	float yUV = 1.0f - fract(actualZ / textureRepeatInterval);


	return vec2(xUV, yUV);
}


vec4 getWallColour(float rayAngle, Wall closestWall, vec2 closestIntersectPoint) {
	const float wallLowZ = -1.0f, wallTopZ = 1.0f;

	vec2 wallUV = getWallUV(closestWall, closestIntersectPoint, wallLowZ, wallTopZ);
	if (wallUV == INVALID) {return INVALIDv4;}

	vec4 thisFragColour;
	if (drawUV > 0) { //DrawUV
		thisFragColour = vec4(wallUV.xy, closestWall.textureID / 32.0f, maxRayDistance);
	} else {
		thisFragColour = texture(textureArray, vec3(wallUV.xy, float(closestWall.textureID)));
	}

	return thisFragColour;
}


bool checkLOS(vec2 pointA, vec2 pointB, int thisIndex=-1, float maxDist=maxRayDistance) {
	Ray LOSRay = createRay(pointA, normalize(pointA-pointB), maxDist);

	for (int index = 0; index < 256; index++) {
		Wall thisWall = walls[index];
		if (thisWall.valid <= 0 || index == thisIndex) {continue; /* Wall is empty, or the wall calling LOS. */}

		vec2 thisIntersectPoint = rayIntersectCheck(LOSRay, thisWall);
		if (thisIntersectPoint == INVALID) {continue; /* Invalid intersect point */}
		if (length(thisIntersectPoint - pointA) + EPSILON >= length(pointA-pointB)) {continue; /* Intersection is beyond the target, ignore it. */}
		return true; //Intersect found.
	}

	return false;
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
	Ray fragRay = createRay(playerPosition.xy, rayDirection);


	float minDistance = maxRayDistance;
	int closestIndex;
	vec2 closestIntersectPoint;

	//Iterate through all the walls.
	for (int index = 0; index < 256; index++) {
		Wall thisWall = walls[index];
		if (thisWall.valid <= 0) {continue; /* Wall is empty */}

		vec2 thisIntersectPoint = rayIntersectCheck(fragRay, thisWall);
		if (thisIntersectPoint == INVALID) {continue; /* Invalid intersect point */}
		float wallDistance = length(playerPosition.xy - thisIntersectPoint);


		if (wallDistance >= minDistance) {continue;}
		
		//Set closest.
		minDistance = wallDistance;
		closestIndex = index;
		closestIntersectPoint = thisIntersectPoint;
	}




	if (minDistance < maxRayDistance) {
		Wall closestWall = walls[closestIndex];
		vec4 albedo = getWallColour(rayAngle, closestWall, closestIntersectPoint);
		if (albedo != INVALIDv4) {
			if (noLighting) {
				fragColour = albedo;

			} else {
				for (int idx=0; idx<64; idx++) {
					//Iterate through all lights.
					Light thisLight = lights[idx];
					if (thisLight.valid <= 0) {continue; /* Light is empty */}
					
					//Shadow Checks
					bool shadow = checkLOS(thisLight.position.xy, closestIntersectPoint, closestIndex, thisLight.intensity);
					vec2 wallDirection = normalize(closestWall.end - closestWall.start);
					vec2 wallNormal = vec2(wallDirection.y, -wallDirection.x); // Default normal
					if (dot(wallNormal, playerPosition.xy - closestIntersectPoint) < 0.0) {
						wallNormal = -wallNormal; // Flip the normal if needed
					}
					vec2 lightDir = normalize(thisLight.position.xy-closestIntersectPoint);
					bool normalCheckPass = dot(wallNormal, lightDir) > 0.0f; //Dot of dir of player-wallIntersect, and intersect-light.

					if (shadow || !normalCheckPass) {
						fragColour = vec4(min(albedo.rgb * DEFAULT_BRIGHTNESS, vec3(1.0f, 1.0f, 1.0f)), 1.0f);
					} else {
						vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
						float distance = length(intersect3D - thisLight.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance.
						float brightness = clamp(attenuation, DEFAULT_BRIGHTNESS, 2.5);

						vec3 lightContribution = thisLight.colour * brightness;
						vec4 litColor = vec4(albedo.rgb * lightContribution, 1.0f);

						fragColour = min(litColor, vec4(1.0f, 1.0f, 1.0f, 1.0f));
					}
				}
			}
		}
		//Save to texture.
		depths[framePosition.x] = minDistance;
	}
	vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
	imageStore(renderedFrame, framePosition, finalFragColour);
}