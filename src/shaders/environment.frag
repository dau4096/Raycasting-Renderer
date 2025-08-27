/* environment.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float verticalFOV;
uniform float maxRayAngle;
uniform float zoomFactor;
uniform bool zoom;
uniform bool useMipMapping;
uniform float currentTime;
uniform bool viewCorrection;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

//Other
uniform int numVisibleVisplanes;
uniform int numWalls;			//Total
uniform int numVisibleWalls; 	//Onscreen
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
	WallIntersect wallIntersects[];
};



struct IntersectionData {
	vec3 position;		//3D intersect location
	vec3 UV;			//UV & texture ID
	float distanceSQ;	//Distance from camera, squared
	uint index;			//The index of the found object
	int foundType;		//The type of the found object
	vec2 normal2D;		//Normal vector of the intersect.
};
#define STACK_SIZE 8
IntersectionData stack[STACK_SIZE];
int topOfStack = 0;

void pushStack(IntersectionData data) {
	int insertIdx = topOfStack;

	for (int i=0; i<topOfStack; i++) {
		if (data.distanceSQ < stack[i].distanceSQ) {
			insertIdx = i;
			break;
		}
	}

	if (topOfStack < STACK_SIZE) {
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
#define blendMipMap true
#define forceMipMapLevel false
#define forcedMipMapLevel 0.0f
#define debugMipMapLevel false

#define mipMapLevels 7.0f
#define minMipMapDistance 5.0f
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

	return fract(yUV + textureOffset.y);
}








//Visplanes
vec2 getVisplaneUV(vec2 position2D, Visplane plane, out int textureID, out bvec4 textureFlags) {
	//1st formatting data;
	unpackTextureFormattingBits1(
		plane.textureData1, textureFlags,
		textureID
	);

	//2nd formatting data;
	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits2(
		plane.textureData2, useWorldSpace,
		invTextureScale, textureOffset
	);

	vec2 worldUV = position2D * invTextureScale;
	vec2 localUV = (position2D - plane.boundingBox.xy) / (plane.boundingBox.xy - plane.boundingBox.zw);

	vec2 UV = mix(localUV, worldUV, vec2(useWorldSpace));
	return fract(UV + textureOffset);
}

vec2 getVertex(Visplane plane, uint index) {
	uint actualIndex = index / 2;
	return (index % 2 == 0) ? plane.vertices[actualIndex].xy : plane.vertices[actualIndex].zw;
}

bool isInsideVP(vec2 point2D, Visplane plane) {
	if (
		((point2D.x < plane.boundingBox.x) || (point2D.x > plane.boundingBox.z)) ||
		((point2D.y < plane.boundingBox.y) || (point2D.y > plane.boundingBox.w))
	) {return false; /* Outside bounding box. */}
	for (uint i=0; i<plane.numVertices; i++) {
		vec2 a = getVertex(plane, i);
		vec2 b = getVertex(plane, (i + 1) % plane.numVertices);
		vec2 edge = b - a;
		vec2 toPoint = point2D - a;
		vec2 normal = vec2(-edge.y, edge.x); //90° Anti-Clockwise

		if (dot(normal, toPoint) < EPSILON) {return false; /* Point is outside the edge */}
	}
	return true;
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
	vec2 rayDelta2D = rayDirection.xy * maxRayDistance;

	Ray fragRay = createRay(playerPosition.xy, rayDirection);

	vec2 rayStart = playerPosition.xy;
	vec2 rayEnd = vec2(fragRay.end.xy);
	
	float normY = (2.0f * fragPosition.y / renderResolution.y) - 1.0f;





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


			float t = (playerPosition.z - thisPlane.height) * invAntiProjection / horizontalScaling;
			if (t < 0.0f || t >= maxRayDistance) {continue; /* Behind origin or out of range. */}
			vec2 intersectPoint = playerPosition.xy + rayDirection * t;

			if (!isInsideVP(intersectPoint, thisPlane)) {continue; /* Outside VP */}

			vec2 d = playerPosition.xy - intersectPoint;
			thisIntersect.distanceSQ = dot(d,d);
			thisIntersect.position = vec3(intersectPoint, thisPlane.height);
			thisIntersect.index = actualIDX;
			thisIntersect.foundType = 2;

			int textureID;
			bvec4 textureFlags;
			vec2 uv = getVisplaneUV(intersectPoint, thisPlane, textureID, textureFlags);
			thisIntersect.UV = (textureFlags.x) ? vec3(uv.yx, textureID) : vec3(uv.xy, textureID);

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
		float minDistance;
		bool trueFound = false;
		while (success) {
			success = popStack(validIntersect);
			
			Wall closestWall;
			Visplane closestPlane;
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
			}


			minDistance = 1.0f / inversesqrt(validIntersect.distanceSQ);
			albedo = fetchUV(validIntersect.UV, minDistance, normal, validIntersect.position);
			if ((albedo.a < 0.5f) && (allowTransparency)) {continue;}
			if (shouldDrawToPositionMap) {
				uint idx = (validIntersect.index << 3) | typeFlag;
				ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
				outFragPosition = vec4(validIntersect.position, float(idx));
				outFragNormal = vec4(normalize(normal.xyz), 1.0f);
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
			outFragColour = vec4(albedo.rgb, minDistance);
			gl_FragDepth = minDistance / maxRayDistance;
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
		outFragPosition = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		outFragNormal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	outFragColour = vec4(skyAlbedo.rgb, 1.0f);
	gl_FragDepth = 1.0f;
}