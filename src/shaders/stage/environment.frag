/* environment.frag */
#version 460 core



//// TEXTURES ////
//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2DArray normalMapArray;
layout(binding=2) uniform sampler2D skyboxTexture;
layout(binding=3) uniform sampler2D portalTexture;
//// TEXTURES ////


//// UNIFORMS ////
//CameraData
uniform float maxRayDistance;
uniform float aspectRatio;
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
//// UNIFORMS ////




//// BUFFERS ////
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
	uint uv;
};
layout(std430, binding=7) buffer wallIntersectSSBO {
	WallIntersect wallIntersects[]; //2D Raycast results;
};
layout(std430, binding=8) buffer visplaneCheckSSBO {
	uvec4 visplaneCheckIndices[]; //Horizontal checks for if visplane could be hit.
};
//// BUFFERS ////



//// GLOBAL ////
vec2 fragPosition;
vec4 fragColour;
float zoomEffect;
double t;
//// GLOBAL ////



//// INCLUDES ////
#include <exct/constants> //For constant values.
#include <exct/packing>   //For bit-packing of data.
#include <exct/stack>     //For the stack of intersects

#define TEXTURE_FETCH
#include <exct/textures>  //For texture read/UV funcs.
#include <exct/walls>
//// INCLUDES ////







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

		uint wallIndex = (intersect.wallIndexAndXUV >> 16u);
		uint numUVRepeats = (intersect.wallIndexAndXUV >> 8u) & 0xFF; //8 bits to represent how many times the wall UV looped in that range.
		float xUV = (intersect.wallIndexAndXUV & 0xFFu) / 255.0f;
		Wall thisWall = walls[wallIndex];
		


		float screenYLow, screenYTop;
		unpackWallProjections(thisWall, intersect.projections, screenYLow, screenYTop);
		if (((fragPosition.y >= screenYTop) || (fragPosition.y <= screenYLow))) {continue; /* Above/Below wall */}
		float t = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);


		float hUVy, lUVy; //Split encoded UV into its actual Y values.
		unpackWallTextureUVs(intersect.uv, hUVy, lUVy);
		float yUV = mix(lUVy, hUVy, fract(t * float(numUVRepeats))); //Interpolate between lower and upper UV scaled by numRepeats to find UV Y component.

		bvec4 textureFlags;
		int textureID;
		unpackTextureFormattingBits1(thisWall.textureData1, textureFlags, textureID);

		thisIntersect.distanceSQ = wallDistanceSQ;
		thisIntersect.index = actualIDX;
		thisIntersect.position = vec3(intersect.position2D, mix(thisWall.start.z, thisWall.end.z, t)); //Interpolate to get this Z.
		thisIntersect.UV = (textureFlags.x) ? vec3(yUV, xUV, textureID) : vec3(xUV, yUV, textureID);
		thisIntersect.foundType = T_WALL;
		thisIntersect.normal2D = intersect.normal2D;

		//Set closest.
		pushStack(thisIntersect);
		foundObject = true;
		
	}


	//Iterate through all visplanes. (3D)
	float antiProjection = 0.5f - (fragPosition.y/renderResolution.y); //Same for every (possible) VP in this frag.
	if (abs(antiProjection) > EPSILON) {/* Avoids DivZero error */
		float invAntiProjection = aspectRatio * zoomEffect / antiProjection;
		uint visplaneStartIndex = uint(framePosition.x * numVisplanes);
		for (int idx=0; idx<numVisibleVisplanes; idx++) {
			//Access visplanes via a buffer containing indices of visible visplanes (could be onscreen.)
			uint actualIDX = visibleVisplaneIndices[idx];
			uint SSBOIndex = visplaneStartIndex + actualIDX;

			uvec4 VPintersectData = visplaneCheckIndices[SSBOIndex];
			uint visplaneValue = VPintersectData.x;

			int screenYTop, screenYLow;
			unpackVPProjections(VPintersectData.y, screenYTop, screenYLow);
			float screenT = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);

			if (((visplaneValue & 0x1) == 0u) || (screenT < 0.0f) || (screenT > 1.0f)) {continue; /* No fragment intersect. */}
			Visplane thisVisplane = visplanes[visplaneValue >> 1];

			if (
				((lowerHalf) && (thisVisplane.height > playerPosition.z)) ||
				((!lowerHalf) && (thisVisplane.height < playerPosition.z))
			) {
				//Frag's ray cannot possibly hit visplane.
				continue;
			}


			float rayT = (playerPosition.z - thisVisplane.height) * invAntiProjection / horizontalScaling;
			if ((rayT < 0.0f) || (rayT >= maxRayDistance)) {continue; /* Behind origin or out of range. */}
			vec2 intersectPoint = playerPosition.xy + rayDirection * rayT;

			vec2 d = playerPosition.xy - intersectPoint; //Shouldn't need to work out some of this, could store upper/lower and interpolate.
			thisIntersect.distanceSQ = dot(d,d);
			thisIntersect.position = vec3(intersectPoint, thisVisplane.height);
			thisIntersect.index = actualIDX;
			thisIntersect.foundType = T_VISPLANE;

			vec2 hUV, lUV;
			unpackVPTextureUVs(VPintersectData.zw, hUV, lUV);
			uint texIDbits = (thisVisplane.textureData1 >> 16u) & 0xFFFu;
			int textureID = (texIDbits == 0xFFFu) ? -1 : int(texIDbits);
			thisIntersect.UV = vec3(
				mix(lUV, hUV, screenT), textureID
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
