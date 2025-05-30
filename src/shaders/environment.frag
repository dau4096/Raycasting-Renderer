/* environment.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform bool zoom;

//Debug
uniform int drawUV;

//Headlamp
uniform bool headLampEnabled;
uniform int headLampFlicker;

//Sun
uniform vec3 sunDirection;
uniform vec3 sunColour;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std140, binding = 7) uniform visplaneUBO {
	Visplane visplanes[128];
};

struct Wall {
	vec3 start;			//Wall Start.
	vec3 end;			//Wall End.
	int textureID;		//Wall Texture.
	int valid;			//Wall Validity.
	float _padding[2];	//Wall Padding.
};
layout(std430, binding = 3) buffer wallUBO {
	Wall walls[512];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 5) uniform lightUBO {
	Light lights[128];
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
float zoomEffect;
float tanVerticalViewAngleOffset;
double t, fragZ;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float MIN_WALL_DIST = 0.125f;
const float DEFAULT_BRIGHTNESS = 0.175f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);

const float textureRepeatInterval = 2.0f;
const bool noLighting = false;


double determinant(dvec2 vecA, dvec2 vecB) {
	return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}


dvec2 rayIntersectCheck(Ray ray, Wall wall) {
	dvec2 wallStartV2 = wall.start.xy;
	dvec2 wallEndV2 = wall.end.xy;

	dvec2 r = ray.end - ray.position;
	dvec2 s = wallEndV2 - wallStartV2;

	double denom = r.x * s.y - r.y * s.x;
	if (abs(denom) < EPSILON) return INVALIDdv2;

	dvec2 diff = wallStartV2 - ray.position;
	double t = (diff.x * s.y - diff.y * s.x) / denom;
	double u = (diff.x * r.y - diff.y * r.x) / denom;

	if (t < 0.0f || u < 0.0f || u > 1.0f) return INVALIDdv2;

	return ray.position + t * r;
}


vec2 getWallUV(Wall thisWall, dvec2 intersectPoint, vec3 originPos) {
	const vec2 UVOffset = vec2(0.5f, 0.0f);
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
	double distance = length(originPos.xy - intersectPoint) / zoomEffect;
	double projectedYLow = (originPos.z - wallLowZ) / distance;
	double projectedYTop = (originPos.z - wallTopZ) / distance;

	double screenYLow = renderResolution.y * (0.5 - projectedYLow);
	double screenYTop = renderResolution.y * (0.5 - projectedYTop);

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2;}

	double a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow); //Alpha to mix by.
	fragZ = mix(wallLowZ, wallTopZ, a);
	double yUV = 1.0f - fract(fragZ / textureRepeatInterval);


	return vec2(xUV, yUV) + UVOffset;
}


vec4 fetchUV(vec3 UV, bool fetchTexture=true) {
	if (drawUV > 0) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture) return vec4(1.0f, 0.0f, 1.0f, 1.0f);
	return texture(textureArray, UV);
}



vec3 getVisplaneIntersect(Visplane plane, vec3 originPos) {
	float targetZ = (originPos.z - plane.height) * zoomEffect;
	vec2 position2D;

	float halfFOV = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -halfFOV + (fragPosition.x / renderResolution.x) * 2.0f * halfFOV;

	float theta = radians(playerViewAngle + rayOffset);
	vec3 rayDirection = vec3(sin(theta), cos(theta), tanVerticalViewAngleOffset);


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

	return vec3(position2D.xy, plane.height);
}


vec2 getVisplaneUV(vec3 position3D) {
	const vec2 UVOffset = vec2(0.5f, 0.5f);
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

	return vec2(xUV, yUV) + UVOffset;
}


bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointB - pointA;
	double distToTargetSQ = dot(LOSDelta.xy, LOSDelta.xy);
	double distToTarget = sqrt(distToTargetSQ);
	dvec3 LOSDirection = normalize(LOSDelta);
	Ray LOSRay = createRay(pointA.xy, normalize(LOSDelta.xy), distToTarget);


	//Iterate through all the walls. (2D)
	for (int idx=0; idx<512; idx++) {
		Wall thisWall = walls[idx];
		if (thisWall.valid <= 0) {break; /* End of valid walls */}
		if (idx == thisIndex && foundType == 1) {continue; /* Wall is empty or is the index calling the LOS check. */}

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
	for (int idx=0; idx<128; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (thisPlane.valid <= 0) {break; /* End of valid visplanes */}
		if (idx == thisIndex && foundType == 2) {continue; /* Visplane is not valid or is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		double tFrac = (thisPlane.height - pointA.z) / LOSDelta.z;
		dvec3 intersectPoint = pointA + LOSDirection * tFrac;
		if ((intersectPoint.x > min(thisPlane.start.x, thisPlane.end.x)) && (intersectPoint.x < max(thisPlane.start.x, thisPlane.end.x)) &&
			(intersectPoint.y > min(thisPlane.start.y, thisPlane.end.y)) && (intersectPoint.y < max(thisPlane.start.y, thisPlane.end.y))) {
			return true;
		}
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


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	float halfFOV = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -halfFOV + (fragPosition.x / renderResolution.x) * 2.0f * halfFOV;
	float rayAngleYaw = radians(playerViewAngle + rayOffset);

	vec2 rayDirection = vec2(sin(rayAngleYaw), cos(rayAngleYaw));
	Ray fragRay = createRay(playerPosition.xy, rayDirection);
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
	tanVerticalViewAngleOffset = tan(normY * (verticalFOV / 2.0f));



	vec3 closestIntersectPoint, closestUV;
	double minDistance = maxRayDistance;
	int closestIndex, foundType = 0;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<512; idx++) {
		Wall thisWall = walls[idx];
		if (thisWall.valid <= 0) {break; /* End of valid walls */}

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
	for (int idx=0; idx<128; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (thisPlane.valid <= 0) {break; /* End of valid visplanes */}


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
				vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
				//Light effect
				for (int idx=0; idx<128; idx++) {
					//Iterate through all lights.
					Light thisLight = lights[idx];
					if (thisLight.valid <= 0) {break; /* End of valid lights */}
					
					//Shadow Checks
					bool inShadow = checkLOS(thisLight.position, closestIntersectPoint, closestIndex, foundType);
					vec3 lightDir = normalize(thisLight.position - closestIntersectPoint);
					float normalDot = dot(normal, lightDir);

					if (!inShadow && normalDot > 0.0f) {
						float distance = length(closestIntersectPoint - thisLight.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance to light.
						float brightness = clamp(attenuation, 0.0f, 1.0f);

						vec3 lightContribution = thisLight.colour * brightness;
						vec4 litColour = vec4(albedo.rgb * lightContribution, 1.0f);

						fragColour += litColour;
					}
				}


				//Headlamp Effect
				if (headLampEnabled) {
					Light headLamp;
					headLamp.position = playerPosition;
					headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
					headLamp.intensity = 5.0f + (headLampFlicker / 768.0f); //headLampFlicker is 0-255.
					headLamp.valid = 1;


					vec3 lightDir = normalize(headLamp.position - closestIntersectPoint);
					float normalDot = dot(normal, lightDir);

					if (normalDot >= 0.0f) {
						float distance = length(closestIntersectPoint - headLamp.position);
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (headLamp.intensity*headLamp.intensity))); //Intensity fades with distance to light.
						float brightness = clamp(attenuation, 0.0f, 1.0f);

						vec3 lightContribution = headLamp.colour * brightness;
						vec4 litColour = vec4(albedo.rgb * lightContribution, 1.0f);

						fragColour += litColour;
					}
				}


				//Sun Effect
				float sunNormalDot = dot(normal, normalize(sunDirection));
				vec3 offset = normal * 0.01f;
				bool sunBlocked = checkLOS(closestIntersectPoint + offset, closestIntersectPoint + offset + sunDirection * maxRayDistance, closestIndex, foundType);
				if ((sunNormalDot >= 0.0f) && !sunBlocked) {
					float sunNormalContrib = (sunNormalDot * 0.5f) + 0.5f;
					vec4 litColour = vec4(albedo.rgb * sunColour.rgb * sunNormalContrib, 1.0f);
					fragColour += litColour;
				}


				//Minimum/Maximum brightness.
				fragColour = clamp(fragColour, albedo * DEFAULT_BRIGHTNESS, albedo * 1.75f);
			}
		}
	} else {
		vec2 UV = vec2(
			fract(rayAngleYaw / 6.28318530718f), //Over 2*Pi.
			1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
		);
		fragColour.rgb = texture(skyboxTexture, UV).rgb;
	}

	
	vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
	imageStore(renderedFrame, framePosition, finalFragColour);
}