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

//Player Data
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
uniform int numSprites;
uniform int numLights;
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

const float mipMapLevels = 7.0f;
const float minMipMapDistance = 5.0f;



vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth) {
	float spriteFootZ = thisSprite.position.z - thisSprite.height/2.0f;
	float spriteHeadZ = thisSprite.position.z + thisSprite.height/2.0f;

	vec2 delta = playerPosition.xy - thisSprite.position.xy;
	float invdistance = inversesqrt(max(dot(delta, delta), 1e-4f));

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


		float spriteDistance = length(playerPosition.xy - thisSprite.position.xy);

		if (spriteDistance >= fragDepth || spriteDistance > maxRayDistance) {continue; /* Too far to see onscreen. */}


		vec2 spriteUV = getSpriteUV(thisSprite, thisSprite.centreX, spriteDistance);
		if (spriteUV == INVALIDv2) {continue; /* Invalid UV, from getSpriteUV() */}
		
		if (drawUV == 1) {
			albedo = vec3(spriteUV.xy, thisSprite.textureID/16);
		} else {
			//Linear, uses MM1 from minMipMapDistance and so on.
			float LODIndex = clamp((mipMapLevels * 2.0f / maxRayDistance) * (float(spriteDistance) - minMipMapDistance), 0.0, mipMapLevels); 
			vec4 alphaTexture = textureLod(textureArray, vec3(spriteUV.xy, float(thisSprite.textureID)), floor(LODIndex));
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
		}

		spriteHit = true;
		closestSprite = thisSprite;
		closestIndex = index;
		fragDepth = spriteDistance;
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