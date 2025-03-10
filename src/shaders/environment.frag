/* environment.frag */
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

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std430, binding = 2) buffer visplaneUBO {
	Visplane visplanes[64];
};

struct Wall {
	vec3 start;			//Wall Start.
	vec3 end;			//Wall End.
	int textureID;		//Wall Texture.
	int valid;			//Wall Validity.
	float _padding[2];	//Wall Padding.
};
layout(std430, binding = 3) buffer wallUBO {
	Wall walls[256];
};

struct Sprite {
	vec2 position;	//Sprite Position.
	float width;	//Sprite Width.
	int textureID;	//Sprite Texture ID.
	int valid;		//Sprite Validity.
	float _padding;	//Memory padding.
};
layout(std140, binding = 4) uniform spriteSSBO {
	Sprite sprites[32];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 5) uniform lightUBO {
	Light lights[64];
};

layout(std430, binding = 6) buffer depthBuffer {
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
float verticalFOV, t;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float MIN_WALL_DIST = 0.125f;
const float DEFAULT_BRIGHTNESS = 0.1f;
const vec2 INVALID = vec2(1e30f, 1e30f);
const vec3 INVALIDv3 = vec3(1e30f, 1e30f, 1e30f);
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
	vec2 wallStartV2 = vec2(wall.start.x, wall.start.y);
	vec2 wallEndV2 = vec2(wall.end.x, wall.end.y);

	vec2 xDiff = vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	vec2 yDiff = vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);


	double divisor = determinant(xDiff, yDiff);
	//If less than some Epsilon value.
	if (abs(divisor) < EPSILON) {
		//Lines do not intersect, as they are nearly parrallel.
		return INVALID;
	}


	vec2 dets = vec2(determinant(ray.position, ray.end), determinant(wallStartV2, wallEndV2));
	double xCoord = determinant(dets, xDiff) / divisor;
	double yCoord = determinant(dets, yDiff) / divisor;

	vec2 intersectPoint = vec2(xCoord, yCoord);


	//Check if the intersection is within the wall segment.
	if ((intersectPoint.x < min(wall.start.x, wall.end.x)) || (intersectPoint.x > max(wall.start.x, wall.end.x)) ||
		(intersectPoint.y < min(wall.start.y, wall.end.y)) || (intersectPoint.y > max(wall.start.y, wall.end.y))) {
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


vec2 getWallUV(Wall thisWall, vec2 intersectPoint) {
	const float textureRepeatInterval = 2.0f;
	vec2 wallStartV2 = vec2(thisWall.start.x, thisWall.start.y);
	vec2 wallEndV2 = vec2(thisWall.end.x, thisWall.end.y);
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//xUV calculation.
	vec2 wallDirection = wallEndV2 - wallStartV2;
	vec2 wallPosition = intersectPoint - wallStartV2;
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


vec4 fetchUV(vec3 UV, bool fetchTexture=true) {
	if (drawUV > 0) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture) return vec4(0.0f, 0.0f, 0.0f, 0.0f);
	return texture(textureArray, UV);
}



vec3 getVisplaneIntersect(float planeHeight) {
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;

	float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));

	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0; // Normalized screen Y [-1, 1]
	float vAO = normY * (verticalFOV/2);
	float theta = radians(playerViewAngle + rayOffset);


	const vec3 planeNormal = vec3(0.0f, 0.0f, 1.0f);
	vec3 rayDirection = normalize(vec3(
		sin(theta), cos(theta),
		tan(radians(vAO))
	));


	float denom = dot(planeNormal, rayDirection);
	if (abs(denom) < EPSILON) {return INVALIDv3; /* Nearly parallel. */}


	vec3 planeOrigin = vec3(0.0f, 0.0f, planeHeight);
	vec3 planeDir = planeOrigin - playerPosition;


	t = (planeHeight - playerPosition.z) / (rayDirection.z * 50.0f);
	if (t <= 0.0f) {return INVALIDv3; /* Behind Ray origin. */}

	vec3 intersectPoint = playerPosition + vec3(rayDirection.xy * t, 0.0f);
	return intersectPoint;
}


vec2 getVisplaneUV(vec3 position3D) {
	bool topHalf = fragPosition.y > renderResolution.y/2;
	vec2 realPosition = position3D.xy;
	float distance = length(realPosition - playerPosition.xy);
	//Compare against some XY bounds maybe.


	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = fract(abs(realPosition.x));
	if (!topHalf) {
		xUV = 1.0f - xUV;
	}
	float yUV = fract(abs(realPosition.y));
	//Texture index depends on top (ceiling) or bottom (floor) half.

	return vec2(xUV, yUV);
}


bool checkLOS(vec2 pointA, vec2 pointB, int thisIndex=-1, float maxDist=maxRayDistance) {
	//2D Checking : not ideal. Change later.
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


	vec3 closestIntersectPoint, closestUV;
	float minDistance = maxRayDistance;
	int closestIndex, foundType = 0;

	//Iterate through all the walls. (2D)
	for (int index = 0; index < 256; index++) {
		Wall thisWall = walls[index];
		if (thisWall.valid <= 0) {continue; /* Wall is empty */}

		vec2 thisIntersectPoint = rayIntersectCheck(fragRay, thisWall);
		if (thisIntersectPoint == INVALID) {continue; /* Invalid intersect point */}
		vec3 thisIntersectPointv3 = vec3(thisIntersectPoint.xy, playerPosition.z);
		float wallDistanceSQ = dot(playerPosition - thisIntersectPointv3, playerPosition - thisIntersectPointv3); //Cheaper length() call

		vec2 wallUV = getWallUV(thisWall, thisIntersectPoint); //Check if inside wall (Valid UV)


		if (wallUV != INVALID && wallDistanceSQ < minDistance*minDistance) {
			//Set closest.
			minDistance = sqrt(wallDistanceSQ);
			closestIndex = index;
			closestIntersectPoint = vec3(thisIntersectPoint.xy, playerPosition.z);
			closestUV = vec3(wallUV.xy, thisWall.textureID);
			foundType = 1;

			if (minDistance <= MIN_WALL_DIST) {
				break; //No closer walls will be found.
			}
		}
		
	}

	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<64; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (thisPlane.valid <= 0) {continue; /* Visplane is not valid. */}


		vec3 intersectPoint = getVisplaneIntersect(thisPlane.height);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			//Out of the range of the Visplane.
			continue;
		}

		float distance = length(intersectPoint - playerPosition);
		if (distance < minDistance) {
			minDistance = distance;
			closestIntersectPoint = intersectPoint;
			closestIndex = idx;
			foundType = 2;
			closestUV = vec3(getVisplaneUV(intersectPoint), thisPlane.textureID);
		}
	}




	if (foundType > 0) { //An intersect was found.
		Wall closestWall;
		Visplane closestPlane;
		vec3 normal;
		if (foundType == 1) { //Wall
			closestWall = walls[closestIndex];

			vec2 wallDirection = normalize(closestWall.end - closestWall.start).xy;
			vec2 normalv2 = vec2(wallDirection.y, -wallDirection.x);
			if (dot(normalv2, playerPosition.xy - closestIntersectPoint.xy) < 0.0) {
				normalv2 = -normalv2;
			}
			normal = vec3(normalv2.xy, 0.0f);

		} else if (foundType == 2) { //Visplane
			closestPlane = visplanes[closestIndex];

			if (closestPlane.height > playerPosition.z) {
				normal = vec3(0.0f, 0.0f, -1.0f);
			} else {
				normal = vec3(0.0f, 0.0f, 1.0f);
			}
		}

		vec4 albedo = fetchUV(closestUV);

		if (albedo != INVALIDv4) {
			if (noLighting || drawUV > 0) {
				fragColour = albedo;

			} else {
				for (int idx=0; idx<64; idx++) {
					//Iterate through all lights.
					Light thisLight = lights[idx];
					if (thisLight.valid <= 0) {continue; /* Light is empty */}
					
					//Shadow Checks
					bool inShadow = checkLOS(thisLight.position.xy, closestIntersectPoint.xy, closestIndex, thisLight.intensity);
					vec3 lightDir = normalize(thisLight.position-closestIntersectPoint);
					bool normalCheckPass = dot(normal, lightDir) > 0.0f; //Dot of dir of player-wallIntersect, and intersect-light.

					if (inShadow || !normalCheckPass) {
						fragColour = vec4(min(albedo.rgb * DEFAULT_BRIGHTNESS, vec3(1.0f, 1.0f, 1.0f)), 1.0f);
					} else {
						vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
						float distance = length(intersect3D - thisLight.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance to light.
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