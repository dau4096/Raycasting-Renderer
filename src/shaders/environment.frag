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
uniform vec2 textureScale;
uniform vec3 textureOffset;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform bool zoom;
uniform ivec2 renderResolution;

//Debug
uniform int drawUV;

//Headlamp
uniform bool headLampEnabled;
uniform int headLampFlicker;

//Sun
uniform vec3 sunDirection;
uniform vec3 sunColour;

//Other
uniform int numVisplanes;
uniform int numWalls;
uniform int numDisplacements;
uniform int numLights;


layout(rgba32f, binding=0) uniform image2D renderedFrame;

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	vec2 _padding;		//Visplane Padding
};
layout(std430, binding=0) buffer visplaneSSBO {
	Visplane visplanes[];
};

struct Wall {
	vec3 start;		//Wall Start.
	vec3 end;		//Wall End.
	vec2 direction;	//Wall 2D Direction
	int textureID;	//Wall Texture.
	float _padding;	//Wall Validity.
};
layout(std430, binding=1) buffer wallSSBO {
	Wall walls[];
};

struct Displacement {
	vec4 vertices[3];
	vec2 UV[3];
	int textureID;
	float _padding;
};
layout(std430, binding=5) buffer displacementSSBO {
	Displacement displacements[];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	bool enabled;		//Light Validity.
	float _padding;		//Light Padding.
};
layout(std430, binding=3) buffer lightSSBO {
	Light lights[];
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
vec2 rayDirectionCentre;
vec4 fragColour;
float zoomEffect;
float halfFOV;
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

const bool noLighting = false;


//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}






vec4 fetchUV(vec3 UV, bool fetchTexture=true) {
	if (drawUV > 0) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture) return vec4(1.0f, 0.0f, 1.0f, 1.0f);
	return texture(textureArray, UV);
}








//Walls
dvec2 rayIntersectCheck(Ray ray, Wall wall, out double t) {
	dvec2 origin = ray.position;
	dvec2 rayDelta = ray.end - ray.position;

	dvec2 wallOrigin = wall.start.xy;
	dvec2 wallDir = wall.end.xy - wallOrigin;

	double rayDeltaCrosswallDirInv = 1.0f / cross2D(rayDelta, wallDir);
	if (abs(rayDeltaCrosswallDirInv) < EPSILON) {return INVALIDdv2; /* Ray and wall are parallel */}

	dvec2 orDir = wallOrigin - origin;
	t = cross2D(orDir, wallDir) * rayDeltaCrosswallDirInv;
	double u = cross2D(orDir, rayDelta) * rayDeltaCrosswallDirInv;

	if (t < 0.0 || u < 0.0 || u > 1.0) return INVALIDdv2;

	return origin + t * rayDelta;
}


bool quickIntersect(vec3 pointA, vec3 pointB, Wall wall) {
	vec2 rayDelta = pointB.xy - pointA.xy;
	vec2 wallDelta = wall.end.xy - wall.start.xy;

	float denom = cross2D(rayDelta, wallDelta);
	if (abs(denom) < 1e-4f) {return false;}

	vec2 rel = wall.start.xy - pointA.xy;
	float t = cross2D(rel, wallDelta) / denom;
	float u = cross2D(rel, rayDelta) / denom;

	if ((t < -1e-4f) || (t > 1.0f + 1e-4f) || (u < -1e-4f) || (u > 1.0f + 1e-4f)) {return false;}

	float z = pointA.z + (pointB.z - pointA.z) * t;

	return (min(wall.start.z, wall.end.z) - 1e-4f <= z) && (z <= max(wall.start.z, wall.end.z) + 1e-4f);
}


vec2 getWallUV(Wall thisWall, dvec2 intersectPoint, vec3 originPos) {
	vec2 wallStartV2 = vec2(thisWall.start.x, thisWall.start.y);
	vec2 wallEndV2 = vec2(thisWall.end.x, thisWall.end.y);
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//xUV calculation.
	double xUV;
	vec2 wallDelta = wallEndV2 - wallStartV2;
	if (abs(wallDelta.y) > abs(wallDelta.x)) {
		xUV = fract(intersectPoint.y / textureScale.x);
	} else {
		xUV = fract(intersectPoint.x / textureScale.x);
	}
	if (xUV < 0.0f) {xUV = 1.0 - abs(xUV);}


	//yUV calculation.
	double distance = length(originPos.xy - intersectPoint) / zoomEffect;
	double projectedYLow = (originPos.z - wallLowZ) / distance;
	double projectedYTop = (originPos.z - wallTopZ) / distance;

	double screenYLow = renderResolution.y * (0.5 - projectedYLow);
	double screenYTop = renderResolution.y * (0.5 - projectedYTop);

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2;}

	double a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow); //Alpha to mix by.
	fragZ = mix(wallLowZ, wallTopZ, a);
	double yUV = 1.0f - fract(fragZ / textureScale.y);


	return vec2(xUV, yUV) + textureOffset.xz;
}








//Displacements
vec2 getScreenPosition(vec3 position3D) {
	vec2 delta = position3D.xy - playerPosition.xy;

	//X
	vec2 direction = normalize(delta);
	float theta = atan(direction.x, direction.y);
	float rayDelta = degrees(theta) - playerViewAngle;
	if (rayDelta > 180.0f) rayDelta -= 360.0f;
	if (rayDelta < -180.0f) rayDelta += 360.0f;
	float X = (renderResolution.x / 2.0f) * ((rayDelta / halfFOV) + 1.0f);

	//Y
	/*
	//Original from getWallUV()
	float projectedYTop = (originPos.z - wallTopZ) / distance;
	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	*/

	float invdistance = inversesqrt(max(dot(delta, delta), 1e-4f));
	float projCentreY = (playerPosition.z - position3D.z) * invdistance;
	float Y = renderResolution.y * (0.5f - projCentreY);

	return vec2(X, Y);
}

float sign2(vec2 a, vec2 b, vec2 c) {
	return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
}
float edge(vec2 a, vec2 b, vec2 c) {
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

bool inDisplacement(vec2 v1, vec2 v2, vec2 v3) {
	float d1 = sign2(fragPosition, v1, v2);
	float d2 = sign2(fragPosition, v2, v3);
	float d3 = sign2(fragPosition, v3, v1);

	bool hasNegative = (d1 < 0) || (d2 < 0) || (d3 < 0);
	bool hasPositive = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(hasNegative && hasPositive);
}

vec3 barycentricWeights(vec2 v1, vec2 v2, vec2 v3) {
	float areaABC = edge(v1, v2, v3);
	float a = edge(fragPosition, v2, v3)/areaABC;
	float b = edge(fragPosition, v3, v1)/areaABC;
	float c = 1.0f - a - b;
	return vec3(a,b,c);
}

bool behindCamera(vec4 pt, vec2 proj) {
	vec2 dir = normalize(pt.xy - playerPosition.xy);
	return (dot(rayDirectionCentre, dir) < 0.0f) || (
		proj.x < 0.0f || proj.x > renderResolution.x ||
		proj.y < 0.0f || proj.y > renderResolution.y
	);
}








//Visplanes
vec3 getVisplaneIntersect(Visplane plane, vec3 originPos, vec2 rayDirection) {
	float targetZ = (originPos.z - plane.height) * zoomEffect;
	vec2 position2D;

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
	bool topHalf = fragPosition.y > renderResolution.y/2;
	vec2 realPosition = position3D.xy;

	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = fract(realPosition.x / textureScale.x);
	if (xUV < 0.0f) {xUV = 1.0f - abs(xUV);}
	if (!topHalf) {
		xUV = 1.0f - xUV;
	}
	float yUV = fract(realPosition.y / textureScale.y);
	if (yUV < 0.0f) {yUV = 1.0f - abs(yUV);}
	//Texture index depends on top (ceiling) or bottom (floor) half.

	return vec2(xUV, yUV) + textureOffset.xy;
}


bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointB - pointA;
	float distToTargetSQ = dot(LOSDelta.xy, LOSDelta.xy);
	float distToTargetInv = inversesqrt(distToTargetSQ);
	dvec3 LOSDirection = LOSDelta * distToTargetInv;
	Ray LOSRay = createRay(pointA.xy, LOSDirection.xy, 1.0f / distToTargetInv);


	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		if (idx == thisIndex && foundType == 1) {continue; /* Wall is empty or is the index calling the LOS check. */}

		bool blocked = quickIntersect(pointA, pointB, thisWall);
		if (blocked) {return true;}
	}


	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<numVisplanes; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (idx == thisIndex && foundType == 2) {continue; /* Visplane is not valid or is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		if (abs(LOSDelta.z) < EPSILON) {continue;}

		double tFrac = (thisPlane.height - pointA.z) / LOSDelta.z;
		if (tFrac <= 0.0 || tFrac >= 1.0) {continue;}

		dvec3 intersectPoint = pointA + LOSDelta * tFrac;
		if (
			intersectPoint.x >= min(thisPlane.start.x, thisPlane.end.x) - EPSILON &&
			intersectPoint.x <= max(thisPlane.start.x, thisPlane.end.x) + EPSILON &&
			intersectPoint.y >= min(thisPlane.start.y, thisPlane.end.y) - EPSILON &&
			intersectPoint.y <= max(thisPlane.start.y, thisPlane.end.y) + EPSILON
		) {
			return true;
		}
	}

	return false;
}








void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);


	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f);
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f);
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	bool lowerHalf = fragPosition.y < (renderResolution.y / 2.0f); //If the frag has no possible way to intersect a visplane below (or above) then skip those.


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	halfFOV = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -halfFOV + (fragPosition.x / renderResolution.x) * 2.0f * halfFOV;
	float rayAngleYaw = radians(playerViewAngle + rayOffset);

	vec2 rayDirection = vec2(sin(rayAngleYaw), cos(rayAngleYaw));
	float rPVA = radians(playerViewAngle);
	rayDirectionCentre = vec2(sin(rPVA), cos(rPVA));
	float distMultiplier = dot(rayDirection, rayDirectionCentre);

	Ray fragRay = createRay(playerPosition.xy, rayDirection);

	vec2 rayStart = playerPosition.xy;
	vec2 rayEnd = vec2(fragRay.end.xy);
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;








	//Find closest intersect
	vec3 closestIntersectPoint, closestUV;
	double minDistanceSQ = maxRayDistance * maxRayDistance;
	int closestIndex, foundType = 0;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		vec2 wallNormal = vec2(-thisWall.direction.y, thisWall.direction.x);
		float projStart = dot(rayStart-thisWall.start.xy, wallNormal);
		float projEnd = dot(rayEnd-thisWall.start.xy, wallNormal);
		if (projStart * projEnd >= 0.0f) {continue; /* Ray never crosses wall. */}


		double t;
		dvec2 intersectPoint = rayIntersectCheck(fragRay, thisWall, t);
		if (intersectPoint == INVALIDdv2) {continue; /* Invalid intersect point */}
		dvec3 intersectPointv3 = dvec3(intersectPoint.xy, playerPosition.z);
		double wallDistanceSQ = dot(playerPosition - intersectPointv3, playerPosition - intersectPointv3); //Cheaper length() call

		vec2 wallUV = getWallUV(thisWall, intersectPoint, playerPosition); //Check if inside wall (Valid UV)


		if (wallUV != INVALIDv2 && wallDistanceSQ < minDistanceSQ) {
			//Set closest.
			minDistanceSQ = wallDistanceSQ;
			closestIndex = idx;
			closestIntersectPoint = vec3(intersectPoint.xy, fragZ);
			closestUV = vec3(wallUV.xy, thisWall.textureID);
			foundType = 1;
		}
		
	}

	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<numVisplanes; idx++) {
		Visplane thisPlane = visplanes[idx];

		if (
			(lowerHalf && thisPlane.height > playerPosition.z) ||
			(!lowerHalf && thisPlane.height < playerPosition.z)
		) {
			//Fragray cannot possibly hit visplane.
			continue;
		}

		vec3 intersectPoint = getVisplaneIntersect(thisPlane, playerPosition, rayDirection);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			//Out of the range of the Visplane.
			continue;
		}

		float vPlaneDistanceSQ = dot(playerPosition.xy - intersectPoint.xy, playerPosition.xy - intersectPoint.xy); //Cheaper length() call
		if (vPlaneDistanceSQ < minDistanceSQ) {
			minDistanceSQ = vPlaneDistanceSQ;
			closestIntersectPoint = intersectPoint;
			closestIndex = idx;
			foundType = 2;
			closestUV = vec3(getVisplaneUV(intersectPoint), thisPlane.textureID);
		}
	}


	//Iterate through all displacements. (pseudo-3D)
	for (int idx=0; idx<numDisplacements; idx++) {
		Displacement thisDisp = displacements[idx];

		vec2 pA = getScreenPosition(thisDisp.vertices[0].xyz);
		vec2 pB = getScreenPosition(thisDisp.vertices[1].xyz);
		vec2 pC = getScreenPosition(thisDisp.vertices[2].xyz);

		if (behindCamera(thisDisp.vertices[0], pA) && behindCamera(thisDisp.vertices[1], pB) && behindCamera(thisDisp.vertices[2], pC)) {continue;}

		if (inDisplacement(pA, pB, pC)) {
			vec3 barycentricW = barycentricWeights(pA, pB, pC);
			vec3 pos3D = (
				thisDisp.vertices[0].xyz * barycentricW.x +
				thisDisp.vertices[1].xyz * barycentricW.y +
				thisDisp.vertices[2].xyz * barycentricW.z
			);
			vec2 delta = pos3D.xy - playerPosition.xy;
			float distanceSQ = dot(delta, delta);

			if (distanceSQ < minDistanceSQ) {
				minDistanceSQ = distanceSQ;
				closestIndex = idx;
				closestIntersectPoint = pos3D;
				foundType = 3;

				vec2 UV = (
					thisDisp.UV[0].xy * barycentricW.x +
					thisDisp.UV[1].xy * barycentricW.y +
					thisDisp.UV[2].xy * barycentricW.z
				);
				closestUV = vec3(UV.xy, thisDisp.textureID);
			}
		}
	}

	double minDistance = distMultiplier * (1.0f / inversesqrt(minDistanceSQ));
	
	
	
	
	
	
	
	//Lighting
	if (foundType > 0) { //An intersect was found.
		Wall closestWall;
		Visplane closestPlane;
		Displacement closestDisp;
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
		} else if (foundType == 3) {
			closestDisp = displacements[closestIndex];

			normal = cross(
				closestDisp.vertices[1].xyz - closestDisp.vertices[0].xyz,
				closestDisp.vertices[2].xyz - closestDisp.vertices[0].xyz
			);

			vec3 pDelta = closestDisp.vertices[0].xyz - playerPosition;
			normal *= -sign(dot(pDelta, normal));
		}

		vec4 albedo = fetchUV(closestUV);

		if (albedo != INVALIDv4) {
			if (noLighting || drawUV > 0) {
				fragColour = albedo;

			} else {
				vec3 intersect3D = vec3(closestIntersectPoint.xy, 0.0f);
				//Light effect
				for (int idx=0; idx<numLights; idx++) {
					//Iterate through all lights.
					Light thisLight = lights[idx];
					if (!thisLight.enabled) {continue;}
					vec3 delta = closestIntersectPoint - thisLight.position;
					float distSQ = dot(delta, delta);
					float attenuation = max(0.0, 1.0 - abs(distSQ / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance to light.
					if (attenuation < 0.0f) {continue;}
					vec3 lightDir = normalize(thisLight.position - closestIntersectPoint);
					float normalDot = dot(normal, lightDir);
					if (normalDot <= EPSILON) {continue;}

					
					//Shadow Checks
					bool inShadow = checkLOS(thisLight.position, closestIntersectPoint, closestIndex, foundType);

					if (!inShadow) {
						vec3 lightContribution = thisLight.colour * attenuation;
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
		vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
		imageStore(renderedFrame, framePosition, finalFragColour);
	} else {
		vec2 UV = vec2(
			fract(rayAngleYaw / 6.28318530718f), //Over 2*Pi.
			1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
		);
		vec3 skyAlbedo = texture(skyboxTexture, UV).rgb;
		imageStore(renderedFrame, framePosition, vec4(skyAlbedo.rgb, maxRayDistance));
	}
}