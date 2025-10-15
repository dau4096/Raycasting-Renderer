/* environment.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2DArray normalMapArray;
layout(binding=2) uniform sampler2D skyboxTexture;
layout(binding=3) uniform sampler2D portalTexture;


//CameraData
uniform float maxRayDistance;
uniform float verticalFOV;
uniform float maxRayAngle;
uniform float zoomFactor;
uniform bool zoom;
uniform bool useMipMapping;
uniform float currentTime;
uniform bool viewCorrection;
uniform int lightingType;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

//Other
uniform uint numWalls;			//Total
uniform uint numVisibleWalls; 	//Onscreen
uniform uint numVisplanes;			//Total
uniform uint numVisibleVisplanes;	//Onscreen
uniform float shadowMapQuality;
uniform bool allowTransparency;


layout(location=0) out vec4 outFragColour;
layout(location=1) out vec4 outFragPosition;
layout(location=2) out vec4 outFragNormal;



struct Visplane {
	vec4 vertices[4];	//2D points
	uint numVertices;	//Number of 2D points
	float height;		//1D height (Z)
	uint textureData1;	//1st Texture formatting data.
	uint textureData2;	//2nd Texture formatting data.
	vec4 boundingBox;	//Bounding box in 2D.
	uint type;			//Visplanetype.
	float extra;		//Extra data.
	uint lightingHandles[4]; //ARB shadowMap handles.
};
layout(std430, binding=0) buffer visplaneSSBO {
	Visplane visplanes[];
};

struct Wall {
	vec3 start;			//3D start point
	vec3 end;			//3D end point
	vec2 direction;		//2D Direction
	uint textureData1;	//1st Texture formatting data.
	uint textureData2;	//2nd Texture formatting data.
	uint type;			//Walltype.
	float extra;		//Extra data.
	uint lightingHandles[8]; //ARB shadowMap handles.
};
layout(std430, binding=1) buffer wallSSBO {
	Wall walls[];
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
	WallIntersect wallIntersects[]; //2D Raycast results;
};
layout(std430, binding=8) buffer visplaneCheckSSBO {
	uint visplaneCheckIndices[]; //Horizontal checks for if visplane could be hit.
};



struct IntersectionData {
	vec3 position;		//3D intersect location
	vec3 UV;			//UV & texture ID
	float distanceSQ;	//Distance from camera, squared
	uint index;			//The index of the found object
	int foundType;		//The type of the found object
	vec2 normal2D;		//Normal vector of the intersect.
	bool isPortal;		//No lighting applied.
};
#define STACK_SIZE 8
IntersectionData stack[STACK_SIZE];
uint topOfStack = 0;

void pushStack(in IntersectionData data) {
	if (topOfStack == 0) {
		stack[0] = data;
		topOfStack = 1;
		return;
	}

	uint insertIdx = topOfStack;
	for (int i=0; i<topOfStack; i++) {
		if (data.distanceSQ < stack[i].distanceSQ) {
			insertIdx = i;
			break;
		}
	}

	if ((topOfStack == STACK_SIZE) && (insertIdx == STACK_SIZE)) {
		return;
	}

	if (topOfStack < STACK_SIZE) {
		topOfStack++;
	}

	for (uint i=topOfStack-1; i>insertIdx; i--) {
		stack[i] = stack[i - 1];
	}

	stack[insertIdx] = data;
}

//Unused;
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
vec4 fragColour;
float zoomEffect;
double t;
float fragZ;
#define INF 0xFFFFFF
#define EPSILON 1e-4f
#define EPSILON_ALT 1e-3f
#define MIN_WALL_DIST 0.125f
#define INVALIDdv2 dvec2(INF, INF)
#define INVALIDv2 vec2(INF, INF)
#define INVALIDv3 vec3(INF, INF, INF)
#define INVALIDv4 vec4(INF, INF, INF, INF)



//////////////// Config stuff ////////////////
//Mip-mapping;
//Debugging for mipmapping
#define MIPMAP_FORCE_LEVEL_ENABLED false
#define MIPMAP_FORCE_LEVEL_VALUE 0.0f
#define MIPMAP_DEBUG false

//Blending between variable numbers of mipmap levels.
#define MIPMAP_BLEND_ENABLED true
#define MIPMAP_LEVELS 7.0f
#define MIPMAP_MIN_DISTANCE 5.0f


//Lighting;
//Any alpha above this value contributes to lighting FBO components.
#define LIGHTING_THRESHOLD_ALPHA 0.75
//////////////// Config stuff ////////////////



void unpackTextureFormattingBits1(
		uint inputBits, out bvec4 textureFlags,
		out int textureID
	) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	- textureFlags; (4 bit flags) [0 - 15]
		1111 0000 0000 0000 0000 0000 0000 0000
	- textureID; (12 bit uint) [0 - 65535]
		0000 1111 1111 1111 0000 0000 0000 0000
	*/
	textureFlags = bvec4(
		bool(inputBits & 0x80000000),
		bool(inputBits & 0x40000000),
		bool(inputBits & 0x20000000),
		bool(inputBits & 0x10000000)
	);

	uint texIDbits = (inputBits >> 16) & 0xFFF;
	textureID = (texIDbits == 0xFFF) ? -1 : int(texIDbits);
}
void unpackTextureFormattingBits2(
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



#define NORMAL_UP vec3(0.0f, 0.0f, 1.0f)

#define T_NONE     0x0
#define T_WALL     0x1
#define T_VISPLANE 0x2

void getNormal(vec3 UV, float LODIndex, inout vec3 surfaceNormal, uint surfaceType) {
	vec3 normalMapValue = textureLod(normalMapArray, UV, LODIndex).xyz;
	switch (surfaceType) {
		case T_WALL: {
			//Tangent-space normals.
			vec3 tangentNormal = normalize(normalMapValue * 2.0f - 1.0f);
			vec3 N = surfaceNormal;
			vec3 T = (abs(N.z) > 0.999f) ? vec3(1.0f, 0.0f, 0.0f) : normalize(cross(NORMAL_UP, N));
			vec3 B = cross(N, T);
		
			//Tangent-space to worldspace.
			surfaceNormal = normalize(mat3(T, B, N) * tangentNormal) * vec3(1.0f, 1.0f, -1.0f);
			break;
		}
		case T_VISPLANE: {
			vec3 thisNormal = normalMapValue * 2.0f - 1.0f;
			surfaceNormal = thisNormal * vec3(1.0f, 1.0f, sign(surfaceNormal.z));
			break;
		}

		default: {
			return;
		}
	}
}


vec4 fetchUV(vec3 UV, double distance, inout vec3 surfaceNormal, vec3 surfacePosition, uint surfaceType) {
	if (debugMode == 1) {
		return vec4(UV.xy, UV.z / 32.0, maxRayDistance);
	}

	if (!useMipMapping) {
		return textureLod(textureArray, UV, 0.0);
	}

	if (MIPMAP_FORCE_LEVEL_ENABLED) {
		return textureLod(textureArray, UV, MIPMAP_FORCE_LEVEL_VALUE);
	}

	//LODIndex takes depth and slope components to be as unobtrusive as possible.
	float depthComponent = (MIPMAP_LEVELS * 2.0 / maxRayDistance) * (float(distance) - MIPMAP_MIN_DISTANCE);
	float slopeComponent = -abs(dot(normalize(playerPosition - surfacePosition), surfaceNormal));
	float LODIndex = clamp(depthComponent + slopeComponent, 0.0, MIPMAP_LEVELS);
	float lod = ceil(LODIndex);


	if (UV.z < 0) {
		//Portal
		return textureLod(portalTexture, fract(UV.xy), lod);
	}

	if (MIPMAP_DEBUG) {
		return vec4(LODIndex / MIPMAP_LEVELS, fract(LODIndex), 0.0, 1.0);
	}

	vec4 mipColour = textureLod(textureArray, UV, lod);
	getNormal(UV, lod, surfaceNormal, surfaceType);

	if (!MIPMAP_BLEND_ENABLED) {
		return mipColour;
	}

	vec4 mipColourLow = textureLod(textureArray, UV, lod - 1.0);
	return mix(mipColourLow, mipColour, fract(LODIndex));
}


vec4 fetchUVIntersect(in IntersectionData thisIntersect, out vec3 surfaceNormal, out bool isPortal) {
	vec3 UV = thisIntersect.UV;
	isPortal = false;
	if (thisIntersect.foundType == T_WALL) {
		surfaceNormal = vec3(thisIntersect.normal2D.xy, 0.0);
		Wall thisWall = walls[thisIntersect.index];
		if (thisWall.type == 15) { //Portal type.
			vec2 surfaceDirection = vec2(-thisIntersect.normal2D.y, thisIntersect.normal2D.x);
			vec2 fragDirection = normalize(thisIntersect.position.xy - playerPosition.xy);
			float dotProd = 0.5f - abs(dot(fragDirection.xy, surfaceDirection.xy));
			UV = vec3(dotProd, thisIntersect.UV.y, -1.0f);
			isPortal = true;
		}

	} else if (thisIntersect.foundType == T_VISPLANE) {
		Visplane thisVisplane = visplanes[thisIntersect.index];
		surfaceNormal = vec3(0.0, 0.0, (thisVisplane.height < playerPosition.z) ? 1.0 : -1.0);
		if (thisVisplane.type == 15) { //Portal type.
			UV = vec3(fract(thisIntersect.position.xy), -1.0f);
			isPortal = true;
		}

	} else {
		return vec4(0.0);
	}

	return fetchUV(
		UV,
		inversesqrt(thisIntersect.distanceSQ),
		surfaceNormal,
		thisIntersect.position,
		thisIntersect.foundType
	);
}





//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}






//Walls
float getWallYUV(Wall thisWall, uint projections, out int textureID, out bvec4 textureFlags) {
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//1st formatting data;
	unpackTextureFormattingBits1(
		thisWall.textureData1, textureFlags,
		textureID
	);

	//2nd formatting data;
	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits2(
		thisWall.textureData2, useWorldSpace,
		invTextureScale, textureOffset
	);


	//The ideal offset is -0x7FFF (-32,767), but they have slight offsets to account for floating-point inconsistencies later. (+/- 1px.)
	float lowOffset = (playerPosition.z < thisWall.start.z) ? 0.0f : -1.0f;
	float screenYLow = float(int((projections >> 16) & 0xFFFF) - 0x7FFF) + lowOffset;

	float topOffset = ((playerPosition.z > thisWall.end.z) ? 0.0f : 1.0f);
	float screenYTop = float(int(projections & 0xFFFF) - 0x7FFF) + topOffset;

	if (fragPosition.y >= screenYTop || fragPosition.y <= screenYLow) {
		return INF;
	}

	float a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);
	fragZ = mix(wallLowZ, wallTopZ, a);

	float yUVWorld = 1.0f - fragZ * invTextureScale.y;
	float yUVLocal = 1.0f - (a * invTextureScale.y);

	float yUV = mix(yUVLocal, yUVWorld, float(useWorldSpace.y));

	return yUV + textureOffset.y;
}








//Visplanes
vec2 getVisplaneUV(vec2 position2D, Visplane plane, out int textureID, out bvec4 textureFlags) {
	//Unpack formatting bits
	unpackTextureFormattingBits1(plane.textureData1, textureFlags, textureID);
	bvec2 useWorldSpace;
	vec2 invTextureScale, textureOffset;
	unpackTextureFormattingBits2(plane.textureData2, useWorldSpace, invTextureScale, textureOffset);

	//Select UV type from bits;
	vec2 uv = mix(
		(position2D - plane.boundingBox.xy) / (plane.boundingBox.xy - plane.boundingBox.zw),	//Local UV
		position2D * invTextureScale,															//World UV
		useWorldSpace
	);

	return uv + textureOffset;
}





void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	//Negative is upward; so subtract.
	float rollDecimal = clamp(degrees(playerViewRoll) / 22.5f, -1.0f, 1.0f) * zoomEffect;
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(degrees(playerViewPitch), -22.5f, 22.5f) * zoomEffect;
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	bool lowerHalf = fragPosition.y < (renderResolution.y / 2.0f); //If the frag has no possible way to intersect a visplane below (or above) then skip those.


	float rayOffset = -maxRayAngle + (fragPosition.x / renderResolution.x) * 2.0f * maxRayAngle;
	float horizontalScaling = (viewCorrection) ? cos(rayOffset) : 1.0f;
	float rayAngleYaw = playerViewAngle + rayOffset;

	vec2 rayDirection = vec2(sin(rayAngleYaw), cos(rayAngleYaw));
	
	float normY = (2.0f * fragPosition.y / renderResolution.y) - 1.0f;





	//Find closest intersect
	IntersectionData thisIntersect;
	thisIntersect.distanceSQ = maxRayDistance * maxRayDistance;
	bool foundObject = false;

	//Iterate through all the walls. (2D)
	uint wallStartIndex = uint(framePosition.x * numWalls);
	for (int idx=0; idx<numVisibleWalls; idx++) {
		//Access walls via a buffer containing indices of visible walls (could be onscreen.)
		uint actualIDX = visibleWallIndices[idx];
		uint SSBOIndex = wallStartIndex + actualIDX;
		WallIntersect intersect = wallIntersects[SSBOIndex];
		float wallDistanceSQ = intersect.distanceSQ;
		if (wallDistanceSQ <= MIN_WALL_DIST * MIN_WALL_DIST) {continue; /* No intersect found, i.e. distance is impossible. */}

		uint wallIndex = (intersect.wallIndexAndXUV >> 8);
		float xUV = (intersect.wallIndexAndXUV & 0xFF) / 255.0f;
		Wall thisWall = walls[wallIndex];
		

		int textureID;
		bvec4 textureFlags;
		float yUV = getWallYUV(thisWall, intersect.projections, textureID, textureFlags); //Check if inside wall (Valid UV)
		if (yUV == INF) {continue; /* Above/Below wall */}

		thisIntersect.distanceSQ = wallDistanceSQ;
		thisIntersect.index = actualIDX;
		thisIntersect.position = vec3(intersect.position2D, fragZ);
		thisIntersect.UV = (textureFlags.x) ? vec3(yUV, xUV, textureID) : vec3(xUV, yUV, textureID);
		thisIntersect.foundType = 1;
		thisIntersect.normal2D = intersect.normal2D;

		//Set closest.
		pushStack(thisIntersect);
		foundObject = true;
		
	}

	float antiProjection = 0.5f - (fragPosition.y/renderResolution.y);
	if (abs(antiProjection) > EPSILON) {/* Avoids DivZero error */
		float invAntiProjection = 1.5f * zoomEffect / antiProjection;
		//Iterate through all visplanes. (3D)
		uint visplaneStartIndex = uint(framePosition.x * numVisplanes);
		for (int idx=0; idx<numVisibleVisplanes; idx++) {
			//Access visplanes via a buffer containing indices of visible visplanes (could be onscreen.)
			uint actualIDX = visibleVisplaneIndices[idx];
			uint SSBOIndex = visplaneStartIndex + actualIDX;
			uint visplaneValue = visplaneCheckIndices[SSBOIndex];
			if ((visplaneValue & 0x1) == 0u) {continue; /* No intersect. */}
			Visplane thisVisplane = visplanes[visplaneValue >> 1];

			if (
				(lowerHalf && thisVisplane.height > playerPosition.z) ||
				(!lowerHalf && thisVisplane.height < playerPosition.z)
			) {
				//Fragray cannot possibly hit visplane.
				continue;
			}


			float t = (playerPosition.z - thisVisplane.height) * invAntiProjection / horizontalScaling;
			if (t < 0.0f || t >= maxRayDistance) {continue; /* Behind origin or out of range. */}
			vec2 intersectPoint = playerPosition.xy + rayDirection * t;

			vec2 minBB = min(thisVisplane.boundingBox.xy, thisVisplane.boundingBox.zw);
			vec2 maxBB = max(thisVisplane.boundingBox.xy, thisVisplane.boundingBox.zw);

			if (
				(intersectPoint.x < minBB.x) || (intersectPoint.x > maxBB.x) ||
				(intersectPoint.y < minBB.y) || (intersectPoint.y > maxBB.y)
			) {
				//Fragray does not hit VP.
				continue;
			}

			vec2 d = playerPosition.xy - intersectPoint;
			thisIntersect.distanceSQ = dot(d,d);
			thisIntersect.position = vec3(intersectPoint, thisVisplane.height);
			thisIntersect.index = actualIDX;
			thisIntersect.foundType = 2;

			int textureID;
			bvec4 textureFlags;
			vec2 uv = getVisplaneUV(intersectPoint, thisVisplane, textureID, textureFlags);
			thisIntersect.UV = vec3(
				mix(uv.xy, uv.xy, textureFlags.x),
				textureID
			);

			pushStack(thisIntersect);
			foundObject = true;
		}
	}
	
	



	vec2 UV = vec2(
		fract(rayAngleYaw / 6.283185f), //Over 2*Pi.
		1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
	);
	vec3 skyAlbedo = texture(skyboxTexture, UV).rgb;

	if (foundObject) { //An intersect was found.
		vec3 mixColour = skyAlbedo;
		IntersectionData closestHalfAlphaIntersect;
		vec3 closestNormal;
		bool foundShadowmappingObject = false;

		for (int index=int(topOfStack)-1; index>=0; index--) {
			uint stackIndex = uint(clamp(index, 0, STACK_SIZE-1));
			IntersectionData stackIntersect = stack[index];
			vec3 thisNormal;
			vec4 surfaceColour = fetchUVIntersect(stackIntersect, thisNormal, stackIntersect.isPortal);
			mixColour = mix(mixColour, surfaceColour.rgb, surfaceColour.a);

			if (surfaceColour.a >= LIGHTING_THRESHOLD_ALPHA) {
				foundShadowmappingObject = true;
				closestHalfAlphaIntersect = stackIntersect;
				closestNormal = thisNormal;
			}
		}


		if (foundShadowmappingObject) {
			gl_FragDepth = 1.0f / (inversesqrt(closestHalfAlphaIntersect.distanceSQ) * maxRayDistance);
		}
		outFragColour = vec4(mixColour.rgb, 1.0f);
		if (shouldDrawToPositionMap) {
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			uint iData;
			if (closestHalfAlphaIntersect.isPortal) {
				iData = (closestHalfAlphaIntersect.index << 3) | (0x4);
				outFragNormal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
			} else {
				iData = (closestHalfAlphaIntersect.index << 3) | (closestHalfAlphaIntersect.foundType & 0x7);
				outFragNormal = vec4(closestNormal.xyz, 0.0f);
			}
			outFragPosition = vec4(closestHalfAlphaIntersect.position.xyz, iData);
		}
		if (foundShadowmappingObject) {return;}
	}


	//No object was hit; the sky should be drawn.
	if (shouldDrawToPositionMap) {
		ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
		outFragPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		outFragNormal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	outFragColour = vec4(skyAlbedo.rgb, 1.0f);
	gl_FragDepth = 1.0f;
}