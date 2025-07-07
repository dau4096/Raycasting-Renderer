/* sprites.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;
uniform bool useMipMapping;

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


layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(rgba32f, binding=1) uniform image2D positionMap;
layout(rgba32f, binding=2) uniform image2D normalMap;


struct Sprite {
	vec3 position;	//Sprite Position.
	float width;	//Sprite Width.
	float height;	//Sprite Height.
	int textureID;	//Sprite Texture ID.
	int centreX;	//Sprite Screen Centre.
};
layout(std430, binding=2) buffer spriteSSBO {
	Sprite sprites[];
};




vec2 fragPosition;
vec4 fragColour;
float fragDepth, tanVerticalViewAngleOffset, zoomEffect;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.25f;
const float HEADLAMP_MIN_LIGHT = 0.1f;
const vec2 INVALIDv2 = vec2(1e30f, 1e30f);
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec3 INVALIDv3 = vec3(1e30f, 1e30f, 1e30f);

const bool blendMipMap = true;
const bool forceMipMapLevel = false;
const float forcedMipMapLevel = 0.0f;
const bool debugMipMapLevel = false;

const float mipMapLevels = 7.0f;
const float minMipMapDistance = 5.0f;



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



vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float invdistance, float depth) {
	float spriteFootZ = thisSprite.position.z - thisSprite.height/2.0f;
	float spriteHeadZ = thisSprite.position.z + thisSprite.height/2.0f;


	float projectedYLow = (playerPosition.z - spriteFootZ) * invdistance * zoomEffect;
	float projectedYTop = (playerPosition.z - spriteHeadZ) * invdistance * zoomEffect;
	
	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	float screenYTop = renderResolution.y * (0.5 - projectedYTop);

	float spriteHeight = screenYTop - screenYLow;
	float spriteWidth = thisSprite.width * spriteHeight;
	spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;



	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteWidth/2);
	float xUV = fract(relativeX / spriteWidth);
	if (fragPosition.x < centrePixelX - (spriteWidth/2) || fragPosition.x >= centrePixelX + (spriteWidth/2)) {return INVALIDv2; /* Horizontally out of sprite bounds */}


	//yUV calculation.
	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2; /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);


	return vec2(xUV, 1.0f - yUV);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);	
	float fragDepth = imageLoad(renderedFrame, framePosition).a;
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f) * zoomEffect;
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
	tanVerticalViewAngleOffset = tan(normY * (verticalFOV / 2.0f));
	zoomEffect = ((zoom) ? zoomFactor : 1.0f);


	vec2 closestUV = INVALIDv2;
	Sprite closestSprite;
	int closestIndex;
	bool spriteHit = false;
	vec3 albedo;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	for (int index=0; index<numSprites; index++) {
		Sprite thisSprite = sprites[index];

		vec2 delta = playerPosition.xy - thisSprite.position.xy;
		float spriteDistanceSQ = dot(delta, delta);

		if (spriteDistanceSQ >= fragDepth*fragDepth || spriteDistanceSQ > maxRayDistance*maxRayDistance) {continue; /* Too far to see onscreen. */}


		float invdistance = inversesqrt(spriteDistanceSQ);
		vec2 spriteUV = getSpriteUV(thisSprite, thisSprite.centreX, invdistance, spriteDistanceSQ);
		if (spriteUV == INVALIDv2) {continue; /* Invalid UV, from getSpriteUV() */}
		
		
		if (debugMode == 1) { //DrawUV
			albedo = vec3(spriteUV.xy, thisSprite.textureID/16);
		} else if (debugMode == 2) { //DrawNormals
			albedo = vec3(delta.xy, 0.0f);
		} else {
			vec4 alphaTexture = fetchUV(vec3(spriteUV.xy, thisSprite.textureID), fragDepth);
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
		}

		fragDepth = 1.0f / invdistance;
		spriteHit = true;
		closestSprite = thisSprite;
		closestIndex = index;
	}





	if (spriteHit) {
		if (shouldDrawToPositionMap) {
			int idx = (closestIndex << 2) | 0x0;
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			imageStore(positionMap, thisFramePosition, vec4(closestSprite.position, float(idx)));
			imageStore(normalMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		imageStore(renderedFrame, framePosition, vec4(albedo.rgb, fragDepth));
	}
}