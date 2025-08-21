/* sprites.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D renderedFrameRO;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;
uniform bool useMipMapping;
uniform bool viewCorrection;

//Player Data
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform bool zoom;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

//Other
uniform int numSprites;
uniform float shadowMapQuality;


layout(rgba32f, binding=0) writeonly uniform image2D renderedFrameWO;
layout(rgba32f, binding=1) writeonly uniform image2D positionMap;
layout(rgba32f, binding=2) writeonly uniform image2D normalMap;


struct Sprite {
	vec3 position;					//Sprite Position.
	float width;					//Sprite Width.
	float height;					//Sprite Height.
	uint textureID_transparency;	//Sprite Texture ID.
	int centreX;					//Sprite Screen Centre.
};
layout(std430, binding=2) buffer spriteSSBO {
	Sprite sprites[];
};




vec2 fragPosition;
vec4 fragColour;
float fragDepth, tanVerticalViewAngleOffset, zoomEffect;
#define PI 3.141592f
#define INF 0xFFFFFF
#define EPSILON 1e-4f
#define EPSILON_ALT 1e-3f
#define DEFAULT_BRIGHTNESS 0.25f
#define HEADLAMP_MIN_LIGHT 0.1f
#define INVALIDv2 vec2(1e30f, 1e30f)
#define INVALIDdv2 dvec2(INF, INF)
#define INVALIDv3 vec3(1e30f, 1e30f, 1e30f)



//////////////// Config stuff ////////////////
//Mip-mapping;
#define blendMipMap true
#define forceMipMapLevel false
#define forcedMipMapLevel 0.0f
#define debugMipMapLevel false

#define mipMapLevels 7.0f
#define minMipMapDistance 5.0f
//////////////// Config stuff ////////////////




vec4 fetchUV(vec3 UV, double distance, bool fetchTexture=true) {
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
	float LODIndex = clamp(depthComponent, 0.0, mipMapLevels); 

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



vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float invdistance) {
	float spriteFootZ = thisSprite.position.z - thisSprite.height/2.0f;
	float spriteHeadZ = thisSprite.position.z + thisSprite.height/2.0f;

	float normPos = -1.0f + 2.0f * clamp(centrePixelX, 0.0f, renderResolution.x) / float(renderResolution.x);
	float distanceOffset = cos(normPos * maxRayAngle);
	if (distanceOffset <= EPSILON_ALT) {return INVALIDv2;}
	float distDiv = (viewCorrection) ? clamp(1.0f / distanceOffset, 1.0f, maxRayDistance) : 1.0f;

	float mult = invdistance * zoomEffect * 1.5f * distDiv;
	float projectedYLow = (playerPosition.z - spriteFootZ) * mult;
	float projectedYTop = (playerPosition.z - spriteHeadZ) * mult;
	
	float screenYLow = renderResolution.y * (0.5f - projectedYLow);
	float screenYTop = renderResolution.y * (0.5f - projectedYTop);

	float spriteHeight = screenYTop - screenYLow;
	float spriteWidth = (thisSprite.width / thisSprite.height) * spriteHeight;
	spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;



	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteWidth/2.0f);
	float xUV = fract(relativeX / spriteWidth);
	if (fragPosition.x < centrePixelX - (spriteWidth/2.0f) || fragPosition.x >= centrePixelX + (spriteWidth/2.0f)) {return INVALIDv2; /* Horizontally out of sprite bounds */}


	//yUV calculation.
	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2; /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);


	return vec2(xUV, 1.0f - yUV);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);	
	vec4 fragData = texture(renderedFrameRO, fragPosition / vec2(renderResolution));
	float fragDepth = fragData.a;
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	//Negative is upward; so subtract.
	float rollDecimal = clamp(degrees(playerViewRoll) / 22.5f, -1.0f, 1.0f) * zoomEffect;
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(degrees(playerViewPitch), -22.5f, 22.5f) * zoomEffect;
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
	tanVerticalViewAngleOffset = tan(normY * (verticalFOV / 2.0f));
	zoomEffect = ((zoom) ? zoomFactor : 1.0f);


	vec2 closestUV = INVALIDv2;
	Sprite closestSprite;
	int closestIndex;
	bool spriteHit = false;
	vec3 albedo;
	float transparency;

	for (int index=0; index<numSprites; index++) {
		Sprite thisSprite = sprites[index];

		vec2 delta = playerPosition.xy - thisSprite.position.xy;
		float spriteDistanceSQ = dot(delta, delta);

		if (spriteDistanceSQ > fragDepth*fragDepth || spriteDistanceSQ > maxRayDistance*maxRayDistance) {continue; /* Too far to see onscreen. */}


		float invdistance = inversesqrt(spriteDistanceSQ);
		vec2 spriteUV = getSpriteUV(thisSprite, thisSprite.centreX, invdistance);
		if (spriteUV == INVALIDv2) {continue; /* Invalid UV, from getSpriteUV() */}
		
		
		uint texID = (thisSprite.textureID_transparency & 0xFFFF);
		if (debugMode == 1) { //DrawUV
			albedo = vec3(spriteUV.xy, texID/64.0f);
		} else if (debugMode == 2) { //DrawNormals
			vec4 alphaTexture = fetchUV(vec3(spriteUV.xy, texID), 1.0f / invdistance);
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = vec3(0.5f, 0.5f, 0.5f);
		} else {
			vec4 alphaTexture = fetchUV(vec3(spriteUV.xy, texID), 1.0f / invdistance);
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
		}

		transparency = (thisSprite.textureID_transparency >> 16) / 65535.0f; //The smallest non-zero value transparency can be is 1/65535.
		fragDepth = 1.0f / invdistance;
		spriteHit = true;
		closestSprite = thisSprite;
		closestIndex = index;
	}





	if (spriteHit) {
		if (shouldDrawToPositionMap && (transparency >= 1.0f)) {
			int idx = (closestIndex << 3) | 0x4;
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			imageStore(positionMap, thisFramePosition, vec4(closestSprite.position, float(idx)));
			imageStore(normalMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		imageStore(renderedFrameWO, framePosition, vec4(mix(fragData.rgb, albedo.rgb, transparency), fragDepth));
	}
}