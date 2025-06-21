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
uniform bool zoom;
uniform vec2 textureScale;
uniform vec3 textureOffset;
uniform bool useMipMapping;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

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
uniform float shadowMapQuality;


layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(rgba32f, binding=1) uniform image2D positionMap;
layout(rgba32f, binding=2) uniform image2D normalMap;


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
	vec4 normal_texID;
};
layout(std430, binding=5) buffer displacementSSBO {
	Displacement displacements[];
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
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);

const bool blendMipMap = true;
const bool forceMipMapLevel = false;
const float forcedMipMapLevel = 0.0f;
const bool debugMipMapLevel = false;

const float mipMapLevels = 7.0f;
const float minMipMapDistance = 5.0f;






vec4 fetchUV(vec3 UV, double distance, vec3 surfaceNormal, vec3 surfacePosition, bool fetchTexture=true) {
	if (debugMode == 1) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture || (UV.z < 0)) return vec4(1.0f, 0.0f, 1.0f, 1.0f);
	if (!useMipMapping) {
		return textureLod(textureArray, UV, 0.0f);
	}
	if (forceMipMapLevel) {
		return textureLod(textureArray, UV, forcedMipMapLevel);
	}

	//Linear, uses MM1 from minMipMapDistance and so on.
	float depthComponent = (mipMapLevels * 2.0f / maxRayDistance) * (float(distance) - minMipMapDistance);
	//Walls facing camera appear sharper, visplanes at grazing angles appear smoother etc.
	float slopeComponent = -abs(dot(normalize(playerPosition - surfacePosition), surfaceNormal));
	float LODIndex = clamp(depthComponent + slopeComponent, 0.0, mipMapLevels); 

	if (debugMipMapLevel) {
		return vec4(LODIndex / mipMapLevels, fract(LODIndex), 0.0f, 1.0f);
	}
	vec4 mipMapColour = textureLod(textureArray, UV, ceil(LODIndex));
	if (!blendMipMap) {
		return mipMapColour;
	}
	vec4 MipMapMinusOneColour = textureLod(textureArray, UV, ceil(LODIndex) - 1.0f);
	return mix(MipMapMinusOneColour, mipMapColour, fract(LODIndex));
}




//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
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
	float projCentreY = (playerPosition.z - position3D.z) * invdistance * zoomEffect;
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
	return (dot(rayDirectionCentre, dir) < 0.0f) || (proj.x < 0.0f || proj.x > renderResolution.x);
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





void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f) * zoomEffect;
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	bool lowerHalf = fragPosition.y < (renderResolution.y / 2.0f); //If the frag has no possible way to intersect a visplane below (or above) then skip those.


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
				closestUV = vec3(UV.xy, thisDisp.normal_texID.w);
			}
		}
	}

	double minDistance = distMultiplier * (1.0f / inversesqrt(minDistanceSQ));
	
	
	



	if (foundType > 0) { //An intersect was found.
		Wall closestWall;
		Visplane closestPlane;
		Displacement closestDisp;
		vec3 normal;
		int typeFlag;
		if (foundType == 1) { //Wall
			closestWall = walls[closestIndex];

			vec2 wallDirection = normalize(closestWall.end - closestWall.start).xy;
			vec2 normalv2 = vec2(wallDirection.y, -wallDirection.x);
			if (dot(normalv2, playerPosition.xy - closestIntersectPoint.xy) < 0.0) {
				normalv2 *= -1;
				closestUV.x *= -1;
			}
			normal = vec3(normalv2.xy, 0.0f);
			typeFlag = 0x1;

		} else if (foundType == 2) { //Visplane
			closestPlane = visplanes[closestIndex];

			if (closestPlane.height > playerPosition.z) {
				normal = vec3(0.0f, 0.0f, -1.0f);
			} else {
				normal = vec3(0.0f, 0.0f, 1.0f);
			}
			typeFlag = 0x2;

		} else if (foundType == 3) {
			closestDisp = displacements[closestIndex];
			normal = closestDisp.normal_texID.xyz;
			typeFlag = 0x3;
		}

		vec4 albedo = fetchUV(closestUV, minDistance, normal, closestIntersectPoint);
		if (shouldDrawToPositionMap) {
			int idx = (closestIndex << 2) | typeFlag;
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			imageStore(positionMap, thisFramePosition, vec4(closestIntersectPoint, float(idx)));
			imageStore(normalMap, thisFramePosition, vec4(normalize(normal.xyz), 1.0f));
		}

		if (debugMode == 2) { //Drawing normals.
			fragColour = vec4((normal.xyz * 0.5f) + 0.5f, 1.0f);
		} else {
			fragColour = albedo;
		}
		vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
		imageStore(renderedFrame, framePosition, finalFragColour);
	} else {
		vec2 UV = vec2(
			fract(rayAngleYaw / 6.28318530718f), //Over 2*Pi.
			1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
		);
		vec3 skyAlbedo = texture(skyboxTexture, UV).rgb;
		if (shouldDrawToPositionMap) {
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			imageStore(positionMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 0.0f));
			imageStore(normalMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		imageStore(renderedFrame, framePosition, vec4(skyAlbedo.rgb, maxRayDistance));
	}
}