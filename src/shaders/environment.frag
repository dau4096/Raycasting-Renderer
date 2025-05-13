/* environment.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform bool zoom;
uniform int drawUV;
uniform bool headLampEnabled;
uniform int headLampFlicker;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 10) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;

	vec2 textureSize;

	float padding[4];
};

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std140, binding = 7) uniform visplaneUBO {
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
layout(std140, binding = 5) uniform lightSSBO {
	Light lights[64];
};




//Ray Struct.
struct Ray {
	dvec2 position, end;
	dvec2 direction;
};

Ray createRay(dvec2 position, dvec2 direction, double maxDist=maxRayDistance) {
	Ray ray;
	ray.position = position;
	ray.direction = direction;
	ray.end = ray.position + (ray.direction.xy * maxDist);
	return ray;
};


vec2 fragPosition;
ivec2 renderResolution;
vec4 fragColour;
double verticalFOV, t, fragZ;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float MIN_WALL_DIST = 0.125f;
const float DEFAULT_BRIGHTNESS = 0.1f;
const float HEADLAMP_MIN_LIGHT = 0.025f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);

const float textureRepeatInterval = 2.0f;
const bool noLighting = false;


double determinant(dvec2 vecA, dvec2 vecB) {
	return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}



float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}


dvec2 rayIntersectCheck(Ray ray, Wall wall) {
	dvec2 wallStartV2 = dvec2(wall.start.x, wall.start.y);
	dvec2 wallEndV2 = dvec2(wall.end.x, wall.end.y);

	dvec2 xDiff = dvec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	dvec2 yDiff = dvec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);


	double divisor = determinant(xDiff, yDiff);
	if (abs(divisor) < EPSILON) {
		//Lines do not intersect, as they are nearly parallel.
		return INVALIDdv2;
	}


	dvec2 dets = dvec2(determinant(ray.position, ray.end), determinant(wallStartV2, wallEndV2));
	double xCoord = determinant(dets, xDiff) / divisor;
	double yCoord = determinant(dets, yDiff) / divisor;

	dvec2 intersectPoint = dvec2(xCoord, yCoord);


	//Check if the intersection is within the wall.
	if ((intersectPoint.x < min(wall.start.x, wall.end.x)) || (intersectPoint.x > max(wall.start.x, wall.end.x)) ||
		(intersectPoint.y < min(wall.start.y, wall.end.y)) || (intersectPoint.y > max(wall.start.y, wall.end.y))) {
		return INVALIDdv2;
	}


	dvec2 intersectDirection = normalize(intersectPoint - ray.position);
	dvec2 directionDifference = ray.direction - intersectDirection;

	
	if (length(directionDifference) < EPSILON_ALT) {
		//Wrong way, behind camera.
		return INVALIDdv2;
	}
	

	return intersectPoint;  
}


vec2 getWallUV(Wall thisWall, dvec2 intersectPoint, vec3 originPos) {
	vec2 wallStartV2 = vec2(thisWall.start.x, thisWall.start.y);
	vec2 wallEndV2 = vec2(thisWall.end.x, thisWall.end.y);
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//xUV calculation.
	double xUV;
	vec2 wallDelta = wallEndV2 - wallStartV2;
	vec2 wallDirection = normalize(wallDelta);
	dvec2 camRight = dvec2(cos(radians(playerViewAngle)), -sin(radians(playerViewAngle)));
	bool flipXUV = dot(wallDirection, camRight) < 0.0f;
	if (abs(wallDelta.y) > abs(wallDelta.x)) {
		xUV = fract(intersectPoint.y / textureRepeatInterval);
	} else {
		xUV = fract(intersectPoint.x / textureRepeatInterval);
	}
	if (xUV < 0.0f) {xUV = 1.0 - abs(xUV);}
	else if (flipXUV) {xUV = 1.0f - xUV;}


	//yUV calculation.
	double zoomEffect = (zoom) ? zoomFactor : 1.0f;
	double distance = length(originPos.xy - intersectPoint) / zoomEffect;
	double projectedYLow = (originPos.z - wallLowZ) / distance;
	double projectedYTop = (originPos.z - wallTopZ) / distance;

	double screenYLow = renderResolution.y * (0.5 - projectedYLow);
	double screenYTop = renderResolution.y * (0.5 - projectedYTop);

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2;}

	double a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow); //Alpha to mix by.
	fragZ = mix(wallLowZ, wallTopZ, a);
	double yUV = 1.0f - fract(fragZ / textureRepeatInterval);


	return vec2(xUV, yUV);
}


vec4 fetchUV(vec3 UV, bool fetchTexture=true) {
	if (drawUV > 0) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture) return vec4(0.0f, 0.0f, 0.0f, 0.0f);
	return texture(textureArray, UV);
}



vec3 getVisplaneIntersect(Visplane plane, vec3 originPos, bool isLOSCheck=false, vec3 LOSDirection=vec3(0.0f, 0.0f, 0.0f)) {
	float zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	float targetZ = (originPos.z - plane.height) * zoomEffect;
	vec2 position2D;

	if (isLOSCheck) { //Used in checkLOS().
		if (abs(LOSDirection.z) < EPSILON) {return INVALIDv3; /* Avoids DivZero error */}
		float t = targetZ / LOSDirection.z;
		if (t < 0.0f) {return INVALIDv3; /* Behind origin */}
		position2D = originPos.xy + LOSDirection.xy * t;


	} else { //Used within the main Visplane loop of main() for rendering.
		float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
		float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;

		float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));

		float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
		float vAO = normY * (verticalFOV/2);
		float theta = radians(playerViewAngle + rayOffset);
		vec3 rayDirection = vec3(sin(theta), cos(theta), tan(vAO));


		/*
		//Original from getWallUV()
		float projectedYTop = (originPos.z - wallTopZ) / distance;
		float screenYLow = renderResolution.y * (0.5 - projectedYLow);
		*/

		float antiProjection = 0.5f - (fragPosition.y/renderResolution.y);
		if (abs(antiProjection) < EPSILON) {return INVALIDv3; /* Avoids DivZero error */}
		float t = targetZ / antiProjection;
		if (t < 0.0f) {return INVALIDv3; /* Behind origin */}
		position2D = originPos.xy + rayDirection.xy * t;
	}

	return vec3(position2D.xy, plane.height);
}


vec2 getVisplaneUV(vec3 position3D) {
	bool topHalf = fragPosition.y > renderResolution.y/2;
	vec2 realPosition = position3D.xy;

	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = fract(realPosition.x / textureRepeatInterval);
	if (xUV < 0.0f) {xUV = 1.0f - abs(xUV);}
	if (!topHalf) {
		xUV = 1.0f - xUV;
	}
	float yUV = fract(realPosition.y / textureRepeatInterval);
	if (yUV < 0.0f) {yUV = 1.0f - abs(yUV);}
	//Texture index depends on top (ceiling) or bottom (floor) half.

	return vec2(xUV, yUV);
}


bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointA - pointB;
	double distToTargetSQ = dot(LOSDelta.xy, LOSDelta.xy);
	double distToTarget = sqrt(distToTargetSQ);
	vec3 LOSDirection = normalize(LOSDelta);
	Ray LOSRay = createRay(pointA.xy, normalize(LOSDelta.xy), distToTarget);


	//Iterate through all the walls. (2D)
	for (int idx = 0; idx < 256; idx++) {
		Wall thisWall = walls[idx];
		if (thisWall.valid <= 0 || (idx == thisIndex && foundType == 1)) {continue; /* Wall is empty or is the index calling the LOS check. */}

		dvec2 intersectPoint = rayIntersectCheck(LOSRay, thisWall);
		if (intersectPoint == INVALIDv2) {continue; /* Invalid intersect point */}

		double distToIntersectSQ = dot(pointA.xy - intersectPoint, pointA.xy - intersectPoint);
		if (distToIntersectSQ > distToTargetSQ) {continue; /* Not within the range of the LOScheck. */}

		double a = sqrt(distToIntersectSQ) / distToTarget;
		double actualZ = mix(pointA.z, pointB.z, a);

		if ((actualZ > min(thisWall.start.z, thisWall.end.z)) && (actualZ < max(thisWall.start.z, thisWall.end.z))) {
			return true;
		}
	}


	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<64; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (thisPlane.valid <= 0 || (idx == thisIndex && foundType == 2)) {continue; /* Visplane is not valid or is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		vec3 intersectPoint = getVisplaneIntersect(thisPlane, pointB, true, LOSDirection);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			continue;
		}
		return true;
	}

	return false;
}


void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);


	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f);
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f);
	fragPosition.y -= pitchDecimal * 10.0f; //10x scaling.


	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;
	float angle = radians(angleClamp(playerViewAngle + 180.0f + rayOffset));

	vec2 rayDirection = vec2(sin(angle), cos(angle));
	Ray fragRay = createRay(playerPosition.xy, rayDirection);


	vec3 closestIntersectPoint, closestUV;
	double minDistance = maxRayDistance;
	int closestIndex, foundType = 0;

	//Iterate through all the walls. (2D)
	for (int idx = 0; idx < 256; idx++) {
		Wall thisWall = walls[idx];
		if (thisWall.valid <= 0) {continue; /* Wall is empty */}

		dvec2 intersectPoint = rayIntersectCheck(fragRay, thisWall);
		if (intersectPoint == INVALIDdv2) {continue; /* Invalid intersect point */}
		dvec3 intersectPointv3 = dvec3(intersectPoint.xy, playerPosition.z);
		double wallDistanceSQ = dot(playerPosition - intersectPointv3, playerPosition - intersectPointv3); //Cheaper length() call

		vec2 wallUV = getWallUV(thisWall, intersectPoint, playerPosition); //Check if inside wall (Valid UV)


		if (wallUV != INVALIDv2 && wallDistanceSQ < minDistance*minDistance) {
			//Set closest.
			minDistance = sqrt(wallDistanceSQ);
			closestIndex = idx;
			closestIntersectPoint = vec3(intersectPoint.xy, fragZ);
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


		vec3 intersectPoint = getVisplaneIntersect(thisPlane, playerPosition);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			//Out of the range of the Visplane.
			continue;
		}

		float vPlaneDistanceSQ = dot(playerPosition.xy - intersectPoint.xy, playerPosition.xy - intersectPoint.xy); //Cheaper length() call
		if (vPlaneDistanceSQ < minDistance*minDistance) {
			minDistance = sqrt(vPlaneDistanceSQ);
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
				normalv2 *= -1;
				closestUV.x *= -1;
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
					bool inShadow = checkLOS(thisLight.position, closestIntersectPoint, closestIndex, foundType);
					vec3 lightDir = normalize(thisLight.position - closestIntersectPoint);
					float normalDot = dot(normal, lightDir);
					float normalEffect = normalDot * 0.6 + 0.4; //Dot of dir of player-wallIntersect, and intersect-light.

					if (inShadow || normalDot < 0.0f) {
						fragColour = vec4(min(albedo.rgb * DEFAULT_BRIGHTNESS + fragColour.rgb, vec3(1.0f, 1.0f, 1.0f)), 1.0f);
					} else {
						vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
						float distance = length(intersect3D - thisLight.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance to light.
						float brightness = clamp(attenuation, DEFAULT_BRIGHTNESS, 2.5);

						vec3 lightContribution = thisLight.colour * brightness;
						vec4 litColor = vec4(albedo.rgb * lightContribution, 1.0f);

						fragColour = min(litColor + fragColour, vec4(1.0f, 1.0f, 1.0f, 1.0f));
					}
				}

				if (headLampEnabled) {
					Light headLamp;
					headLamp.position = playerPosition;
					headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
					headLamp.intensity = 5.0f + (headLampFlicker / 768.0f); //headLampFlicker is 0-255.
					headLamp.valid = 1;


					vec3 lightDir = normalize(headLamp.position - closestIntersectPoint);
					float normalDot = dot(normal, lightDir);
					float normalEffect = normalDot * 0.6 + 0.4; //Dot of dir of player-wallIntersect, and intersect-light.

					if (normalDot < 0.0f) {
						fragColour = vec4(min(albedo.rgb * HEADLAMP_MIN_LIGHT + fragColour.rgb, vec3(1.0f, 1.0f, 1.0f)), 1.0f);
					} else {
						vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
						float distance = length(intersect3D - headLamp.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (headLamp.intensity*headLamp.intensity))); //Intensity fades with distance to light.
						float brightness = clamp(attenuation, HEADLAMP_MIN_LIGHT, 2.5);

						vec3 lightContribution = headLamp.colour * brightness;
						vec4 litColor = vec4(albedo.rgb * lightContribution, 1.0f);

						fragColour = min(litColor + fragColour, vec4(1.0f, 1.0f, 1.0f, 1.0f));
					}
				}
			}
		}
	}

	
	vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
	imageStore(renderedFrame, framePosition, finalFragColour);
}