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
	float _padding;
};
layout(std430, binding=7) buffer wallIntersectSSBO {
	WallIntersect wallIntersects[]; //2D Raycast results;
};
layout(std430, binding=8) buffer visplaneCheckSSBO {
	uvec2 visplaneCheckIndices[]; //Horizontal checks for if visplane could be hit.
};
//Number of walls and number of visplanes found, in an atomic uint. (alternates walls and vps)
layout(std430, binding=10) buffer numFoundObjectsAtomicSSBO {uint numFoundObjects[];};
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
#include <exct/visplanes>
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
	for (int idx=0; idx<numFoundObjects[framePosition.x * 2]; idx++) { //numFoundObjects[framePosition.x * 2]
		//Access walls via a buffer containing indices of 2D-raycasted wall intersections.
		uint SSBOIndex = wallStartIndex + idx;
		WallIntersect intersect = wallIntersects[SSBOIndex];
		float wallDistanceSQ = intersect.distanceSQ;

		uint wallIndex = (intersect.wallIndexAndXUV >> 8u);
		float xUV = (intersect.wallIndexAndXUV & 0xFFu) / 255.0f;
		Wall thisWall = walls[wallIndex];
		

		int textureID;
		bvec4 textureFlags;
		float fragZ;
		float yUV = getWallYUV(thisWall, intersect.projections, textureID, textureFlags, fragPosition, fragZ); //Check if inside wall (Valid UV)
		if (yUV == INF) {continue; /* Above/Below wall */}

		thisIntersect.distanceSQ = wallDistanceSQ;
		thisIntersect.index = wallIndex;
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
		float invAntiProjection = aspectRatio * zoomEffect / antiProjection;
		//Iterate through all visplanes. (3D)
		uint visplaneStartIndex = uint(framePosition.x * numVisplanes);
		for (int idx=0; idx<numFoundObjects[framePosition.x * 2 + 1]; idx++) { //numFoundObjects[framePosition.x * 2 + 1]
			//Access visplanes via a buffer containing indices of 2D-raycasted visplane intersections.
			uint SSBOIndex = visplaneStartIndex + idx;
			uvec2 VPintersectData = visplaneCheckIndices[SSBOIndex];

			int higherProj = int(VPintersectData.y >> 16u) - 0x7FFF;
			int lowerProj = int(VPintersectData.y & 0xFFFFu) - 0x7FFF;

			if ((fragPosition.y < lowerProj) || (fragPosition.y > higherProj)) {continue; /* No intersect. */}
			uint vpIndex = VPintersectData.x >> 1;
			Visplane thisVisplane = visplanes[vpIndex];

			if (
				((lowerHalf) && (thisVisplane.height > playerPosition.z)) ||
				((!lowerHalf) && (thisVisplane.height < playerPosition.z))
			) {
				//Frag's ray cannot possibly hit visplane.
				continue;
			}


			float t = (playerPosition.z - thisVisplane.height) * invAntiProjection / horizontalScaling;
			if ((t < 0.0f) || (t >= maxRayDistance)) {continue; /* Behind origin or out of range. */}
			vec2 intersectPoint = playerPosition.xy + rayDirection * t;

			vec2 d = playerPosition.xy - intersectPoint;
			thisIntersect.distanceSQ = dot(d,d);
			thisIntersect.position = vec3(intersectPoint, thisVisplane.height);
			thisIntersect.index = vpIndex;
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