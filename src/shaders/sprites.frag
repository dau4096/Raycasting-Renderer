/* sprites.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec2 playerPosition;
uniform bool zoom;
uniform int drawUV;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
    float zoomFactor;
    float maxRayAngle;
    float maxRayDistance;

    float topIndex;
    float lowIndex;

    vec2 textureSize;

    float padding[2];
};



struct Sprite {
	vec2 position;	//Sprite Position.
	float width;	//Sprite Width.
	int textureID;	//Sprite Texture ID.
	int valid;		//Sprite Validity.
	float _padding; //Memory padding.
};
layout(std140, binding = 3) uniform spriteSSBO {
	Sprite sprites[128];
};


struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float padding[3];   //Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[32];
};
layout(std430, binding = 5) buffer depthBuffer {
	float depths[];
};


vec2 fragPosition;
ivec2 renderResolution;
vec4 fragColour;
float fragDepth;


vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth, vec2 spriteDimentions) {
	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteDimentions.x/2);
	float xUV = fract(relativeX / spriteDimentions.x);
	if (fragPosition.x < centrePixelX - (spriteDimentions.x/2) || fragPosition.x >= centrePixelX + (spriteDimentions.x/2)) {return vec2(1e30f, 1e30f); /* Horizontally out of sprite bounds */}


	//yUV calculation.
	int midPointY = renderResolution.y / 2;
	float yCoordScreen = midPointY + fragPosition.y;
	float spriteTop = midPointY - spriteDimentions.y / 2.0f;
	float spriteBottom = midPointY + spriteDimentions.y / 2.0f;


	//Don't allow drawing above/below wall top/bottom respectively.
	if (fragPosition.y < spriteTop || fragPosition.y > spriteBottom) {return vec2(1e30f, 1e30f); /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - (midPointY - spriteDimentions.y / 2.0f)) / spriteDimentions.y;


	return vec2(xUV, 1.0f - yUV);
}



float angleClamp(float value) {
	if (value < 0.0f) {
		return angleClamp(360.0f + value);
	}
	return mod(value, 360.0f);
}


float getSpriteScreenX(Sprite thisSprite, float rayAngle) {
	float f = tan(radians(rayAngle)); //tan(FOV/2)
	float a = radians(playerViewAngle);

	vec2 dir = vec2(sin(a), cos(a));
	vec2 plane = vec2(-cos(a) * f, sin(a) * f);
	vec2 spriteDir = thisSprite.position - playerPosition;

	if (dot(dir, normalize(spriteDir)) < 0.0f) {return 1e30f;}


	float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

	float transformX = invDet * (dir.y * spriteDir.x - dir.x * spriteDir.y);
	float transformY = invDet * (-plane.y * spriteDir.x + plane.x * spriteDir.y);

	return (renderResolution.x / 2.0f) * (1.0f - transformX / transformY);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);	
	float fragDepth = imageLoad(renderedFrame, framePosition).a;


	vec2 closestUV = vec2(1e30f, 1e30f);
	Sprite closestSprite;
	bool spriteHit = false;
	vec3 fragColour = vec3(0.0f, 0.0f, 0.0f);
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	for (int index = 0; index < 32; index++) {
		Sprite thisSprite = sprites[index];
		if (thisSprite.valid <= 0) {continue; /* Sprite is empty */}


		float spriteDistance = length(playerPosition - thisSprite.position);

		if (spriteDistance >= fragDepth || spriteDistance > maxRayDistance) {continue; /* Too far to see onscreen. */}


		float spriteMaxHeight = 0.8f;

		float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));
		float viewAngleOffset = atan(spriteMaxHeight/spriteDistance);
		float spriteHeight = (renderResolution.y * 2 * viewAngleOffset) / (verticalFOV * ((zoom) ? zoomFactor : 1.0f));

		float spriteWidth = thisSprite.width * spriteHeight;

		spriteWidth = (zoom) ? spriteWidth * zoomFactor : spriteWidth;
		spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;
		vec2 spriteDimentions = vec2(spriteWidth, spriteHeight);

		float centrePixelX = getSpriteScreenX(thisSprite, rayAngle);
		if (centrePixelX == 1e30f) {continue; /* Invalid cpX, probably offscreen. */}


		vec2 spriteUV = getSpriteUV(thisSprite, centrePixelX, spriteDistance, spriteDimentions);
		if (spriteUV == vec2(1e30f, 1e30f)) {continue; /* Invalid UV, from getSpriteUV() */}
		
		if (drawUV == 1) {
			fragColour = vec3(spriteUV.xy, thisSprite.textureID/16);
		} else {
			vec4 alphaTexture = texture(textureArray, vec3(spriteUV.xy, float(thisSprite.textureID)));
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			fragColour = alphaTexture.rgb;
			spriteHit = true;
		}

		fragDepth = spriteDistance;
	}



	if (!spriteHit) {return; /* fragment does not intersect with a valid point on a sprite. */}
	//Only write the final pixel to the frame.

	vec4 finalFragColour = vec4(fragColour, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}