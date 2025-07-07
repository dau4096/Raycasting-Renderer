/* environment.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float zoomFactor;
uniform bool zoom;
uniform bool useMipMapping;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

//Other
uniform int numDisplacements;
uniform int numVisibleVisplanes;
uniform int numWalls;			//Total
uniform int numVisibleWalls; 	//Onscreen
uniform float shadowMapQuality;
uniform bool allowTransparency;


layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(rgba32f, binding=1) uniform image2D positionMap;
layout(rgba32f, binding=2) uniform image2D normalMap;


struct Visplane {
	vec2 start;			//2D start point
	vec2 end;			//2D end point
	float height;		//1D height (Z)
	int textureID;		//Texture ID
	uint textureData;	//Texture formatting data.
	float _padding;		//Buffer Padding
};
layout(std430, binding=0) buffer visplaneSSBO {
	Visplane visplanes[];
};

struct Wall {
	vec3 start;			//3D start point
	vec3 end;			//3D end point
	vec2 direction;		//2D Direction
	int textureID;		//Texture ID
	uint textureData;	//Texture formatting data
};
layout(std430, binding=1) buffer wallSSBO {
	Wall walls[];
};

struct Displacement {
	vec4 vertices[3];	//3D Vertices
	vec2 UV[3];			//2D UV coordinates per vertex
	vec4 normal_texID;	//Normal and texture ID packed together
};
layout(std430, binding=4) buffer displacementSSBO {
	Displacement displacements[];
};

//Buffers containing indices of all valid objects (referencing their actual datasets above.)
layout(std430, binding=5) buffer visibleVisplaneIndicesSSBO {uint visibleVisplaneIndices[];};
layout(std430, binding=6) buffer visibleWallIndicesSSBO {uint visibleWallIndices[];};


//All 2D intersects found from raycast.comp previously.
struct WallIntersect {
	vec2 position2D;
	vec2 normal2D;
	uint projections;
	uint wallIndexAndXUV;
	float distanceSQ;
	float _padding;
};
layout(std430, binding=7) buffer wallIntersectSSBO {
	WallIntersect wallIntersects[];
};



struct IntersectionData {
	vec3 position;		//3D intersect location
	vec3 UV;			//UV & texture ID
	double distanceSQ;	//Distance from camera, squared
	uint index;			//The index of the found object
	int foundType;		//The type of the found object
	vec2 normal2D;		//Normal vector of the intersect.
};
IntersectionData stack[3];
int topOfStack = 0;

void pushStack(IntersectionData data) {
	int insertIdx = topOfStack;

	for (int i=0; i<topOfStack; i++) {
		if (data.distanceSQ < stack[i].distanceSQ) {
			insertIdx = i;
			break;
		}
	}

	if (topOfStack < 3) {
		topOfStack++;
	}
	for (int i=topOfStack-1; i>insertIdx; i--) {
		stack[i] = stack[i-1];
	}

	stack[insertIdx] = data;
}


bool popStack(out IntersectionData data) {
	if (topOfStack > 0) {
		data = stack[0];
		for (int i=0; i<topOfStack - 1; i++) {
			stack[i] = stack[i+1];
		}
		topOfStack--;
		return true;
	}
	return false;
}




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
double t;
float fragZ;
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





void unpackTextureFormattingBits(
		uint inputBits, out bvec2 isWorldspace,
		out vec2 invTextureScale, out vec2 textureOffset
	) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	isWorldspace.x; (bool) [0 / 1]
		1000 0000 0000 0000 0000 0000 0000 0000
	- isWorldspace.y; (bool) [0 / 1]
		0100 0000 0000 0000 0000 0000 0000 0000
	- textureScale.x; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0011 1111 1100 0000 0000 0000 0000 0000
	- textureScale.y; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0000 0000 0011 1111 1100 0000 0000 0000
	- textureOffset.x; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0011 1111 1000 0000
	- textureOffset.y; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0000 0000 0111 1111
	*/

	isWorldspace = bvec2(
		bool(inputBits & 0x80000000),
		bool(inputBits & 0x40000000)
	);

	invTextureScale = 16.0f / vec2(
		float((inputBits >> 22) & 0xFF),
		float((inputBits >> 14) & 0xFF)
	);

	textureOffset = vec2(
		float((inputBits >> 7) & 0x7F),
		float((inputBits >> 0) & 0x7F)
	) / 128.0f;
}





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
float getWallYUV(Wall thisWall, uint projections) {
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;


	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits(
		thisWall.textureData, useWorldSpace,
		invTextureScale, textureOffset
	);


	float screenYLow = float((projections >> 16) & 0xFFFF) - 12289.0f; //12,288 == 0x3000
	float screenYTop = float(projections & 0xFFFF) - 12287.0f;

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {
		return INF;
	}

	float a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);
	fragZ = mix(wallLowZ, wallTopZ, a);

	float yUVWorld = 1.0f - fragZ * invTextureScale.y;
	float yUVLocal = 1.0f - (a * invTextureScale.y);

	float yUV = mix(yUVLocal, yUVWorld, float(useWorldSpace.y));

	return fract(yUV + textureOffset.y);
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

	float invDistance = inversesqrt(max(dot(delta, delta), 1e-4f));
	float projCentreY = (playerPosition.z - position3D.z) * invDistance * zoomEffect;
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
vec2 getVisplaneUV(vec3 position3D, Visplane plane) {
	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits(
		plane.textureData, useWorldSpace,
		invTextureScale, textureOffset
	);

	vec2 pos = position3D.xy;
	vec2 localUV = (pos - plane.start.xy) / (plane.end.xy - plane.start.xy);
	vec2 worldUV = pos * invTextureScale;

	vec2 UV = mix(localUV * invTextureScale, worldUV, vec2(useWorldSpace));
	return UV - floor(UV) + textureOffset;
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
	vec2 rayDelta2D = rayDirection.xy * maxRayDistance;
	float rPVA = radians(playerViewAngle);
	rayDirectionCentre = vec2(sin(rPVA), cos(rPVA));
	float distMultiplier = dot(rayDirection, rayDirectionCentre);

	Ray fragRay = createRay(playerPosition.xy, rayDirection);

	vec2 rayStart = playerPosition.xy;
	vec2 rayEnd = vec2(fragRay.end.xy);
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;








	//Find closest intersect
	IntersectionData thisIntersect;
	thisIntersect.distanceSQ = maxRayDistance * maxRayDistance;
	bool foundObject = false;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numVisibleWalls; idx++) {
		//Access walls via a buffer containing indices of visible walls (could be onscreen.)
		uint actualIDX = visibleWallIndices[idx];
		uint SSBOIndex = uint(framePosition.x * numWalls) + actualIDX;
		WallIntersect intersect = wallIntersects[SSBOIndex];
		float wallDistanceSQ = intersect.distanceSQ;
		if (wallDistanceSQ <= MIN_WALL_DIST * MIN_WALL_DIST) {continue; /* No intersect found, i.e. distance is impossible. */}

		uint wallIndex = (intersect.wallIndexAndXUV >> 8);
		float xUV = (intersect.wallIndexAndXUV & 0xFF) / 255.0f;
		Wall thisWall = walls[wallIndex];
		

		float yUV = getWallYUV(thisWall, intersect.projections); //Check if inside wall (Valid UV)
		if (yUV == INF) {continue; /* Above/Below wall */}

		thisIntersect.distanceSQ = wallDistanceSQ;
		thisIntersect.index = actualIDX;
		thisIntersect.position = vec3(intersect.position2D, fragZ);
		thisIntersect.UV = vec3(xUV, yUV, thisWall.textureID);
		thisIntersect.foundType = 1;
		thisIntersect.normal2D = intersect.normal2D;

		//Set closest.
		pushStack(thisIntersect);
		foundObject = true;
		
	}

	float antiProjection = 0.5f - (fragPosition.y/renderResolution.y);
	if (abs(antiProjection) > EPSILON) {/* Avoids DivZero error */
		float invAntiProjection = zoomEffect / antiProjection;
		//Iterate through all visplanes. (3D)
		for (int idx=0; idx<numVisibleVisplanes; idx++) {
			//Access visplanes via a buffer containing indices of visible visplanes (could be onscreen.)
			uint actualIDX = visibleVisplaneIndices[idx];
			Visplane thisPlane = visplanes[actualIDX];

			if (
				(lowerHalf && thisPlane.height > playerPosition.z) ||
				(!lowerHalf && thisPlane.height < playerPosition.z)
			) {
				//Fragray cannot possibly hit visplane.
				continue;
			}

			float t = (playerPosition.z - thisPlane.height) * invAntiProjection;
			if (t < 0.0f || t >= maxRayDistance) {continue; /* Behind origin or out of range. */}
			vec3 intersectPoint = vec3(playerPosition.xy + rayDirection.xy * t, thisPlane.height);

			vec2 planeMin = min(thisPlane.start, thisPlane.end);
			vec2 planeMax = max(thisPlane.start, thisPlane.end);
			if ((intersectPoint.x < planeMin.x) || (intersectPoint.x > planeMax.x) ||
				(intersectPoint.y < planeMin.y) || (intersectPoint.y > planeMax.y)) {
				//Out of the range of the Visplane.
				continue;
			}

			float dx = playerPosition.x - intersectPoint.x;
			float dy = playerPosition.y - intersectPoint.y;
			thisIntersect.distanceSQ = dx * dx + dy * dy;
			thisIntersect.position = intersectPoint;
			thisIntersect.index = actualIDX;
			thisIntersect.foundType = 2;
			thisIntersect.UV = vec3(getVisplaneUV(intersectPoint, thisPlane), thisPlane.textureID);
			pushStack(thisIntersect);
			foundObject = true;
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

			
			thisIntersect.distanceSQ = distanceSQ;
			thisIntersect.index = idx;
			thisIntersect.position = pos3D;
			thisIntersect.foundType = 3;

			vec2 UV = (
				thisDisp.UV[0].xy * barycentricW.x +
				thisDisp.UV[1].xy * barycentricW.y +
				thisDisp.UV[2].xy * barycentricW.z
			);
			thisIntersect.UV = vec3(UV.xy, thisDisp.normal_texID.w);
			pushStack(thisIntersect);
			foundObject = true;
		}
	}


	
	



	if (foundObject) { //An intersect was found.
		IntersectionData validIntersect;
		bool success = true;
		vec3 normal;
		int typeFlag;
		vec4 albedo;
		double minDistance;
		bool trueFound = false;
		while (success) {
			success = popStack(validIntersect);
			
			Wall closestWall;
			Visplane closestPlane;
			Displacement closestDisp;
			if (validIntersect.foundType == 1) { //Wall
				closestWall = walls[validIntersect.index];

				vec2 wallDirection = normalize(closestWall.end - closestWall.start).xy;
				vec2 normalv2 = vec2(wallDirection.y, -wallDirection.x);
				if (dot(normalv2, playerPosition.xy - validIntersect.position.xy) < 0.0) {
					normalv2 *= -1;
					validIntersect.UV.x *= -1;
				}
				normal = vec3(validIntersect.normal2D.xy, 0.0f);
				typeFlag = 0x1;

			} else if (validIntersect.foundType == 2) { //Visplane
				closestPlane = visplanes[validIntersect.index];

				if (closestPlane.height > playerPosition.z) {
					normal = vec3(0.0f, 0.0f, -1.0f);
				} else {
					normal = vec3(0.0f, 0.0f, 1.0f);
				}
				typeFlag = 0x2;

			} else if (validIntersect.foundType == 3) {
				closestDisp = displacements[validIntersect.index];
				normal = closestDisp.normal_texID.xyz;
				typeFlag = 0x3;
			}

			minDistance = distMultiplier / inversesqrt(validIntersect.distanceSQ);
			albedo = fetchUV(validIntersect.UV, minDistance, normal, validIntersect.position);
			if ((albedo.a < 0.5f) && (allowTransparency)) {continue;}
			if (shouldDrawToPositionMap) {
				uint idx = (validIntersect.index << 2) | typeFlag;
				ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
				imageStore(positionMap, thisFramePosition, vec4(validIntersect.position, float(idx)));
				imageStore(normalMap, thisFramePosition, vec4(normalize(normal.xyz), 1.0f));
			}
			trueFound = true;
			break;

		}

		if (trueFound) {
			if (debugMode == 2) { //Drawing normals.
				fragColour = vec4((normal.xyz * 0.5f) + 0.5f, 1.0f);
			} else {
				fragColour = albedo;
			}
			vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
			imageStore(renderedFrame, framePosition, finalFragColour);
			return;
		}
	}

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