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
	float dimmingStrength;

	float toRad;

	vec3 topColour;
	vec3 lowColour;

	float padding[3];
};


struct Sprite {
	vec2 position;		//Sprite Position.
	float width;		//Sprite Width.
	int textureID;		//Sprite Texture.
	int valid;			//Sprite Validity.
	float padding[2];	//Sprite Padding.
};

layout(std140, binding = 3) uniform spriteUBO {
	Sprite sprites[32];
};


struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float padding[2];   //Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[32];
};


vec2 fragPosition;
ivec2 screenDimentions;
vec4 fragColour;
float fragDepth;


vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth, vec2 spriteDimentions) {
	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteDimentions.x/2);
	if (relativeX < 0.0f || relativeX >= centrePixelX + (spriteDimentions.x/2)) {return vec2(1e30f, 1e30f); /* Outside of sprite horizontal bounds */}
	float xUV = fract(relativeX / spriteDimentions.x);
	if (fragPosition.x < centrePixelX - (spriteDimentions.x/2) || fragPosition.x >= centrePixelX + (spriteDimentions.x/2)) {return vec2(1e30f, 1e30f); /* Horizontally out of sprite bounds */}


	//yUV calculation.
	int midPointY = screenDimentions.y / 2;
	float yCoordScreen = midPointY + fragPosition.y;
	float spriteTop = midPointY - spriteDimentions.y / 2.0f;
	float spriteBottom = midPointY + spriteDimentions.y / 2.0f;


	//Don't allow drawing above/below wall top/bottom respectively.
	if (fragPosition.y < spriteTop || fragPosition.y > spriteBottom) {return vec2(1e30f, 1e30f); /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - (midPointY - spriteDimentions.y / 2.0f)) / spriteDimentions.y;


	return vec2(xUV, 1.0f - yUV);
}



float getSpriteScreenX(Sprite sprite, float onScreenWidth) {
	//Make sure to return a VERY offscreen x coordinate to be interpreted as "Invalid" (-1e3f)
	vec2 spriteDirection = normalize(sprite.position - playerPosition); //Direction from player to sprite.
	vec2 playerDirection = normalize(vec2(sin(radians(playerViewAngle)), cos(radians(playerViewAngle)))); //View direction.

	float dot = dot(spriteDirection, playerDirection);
	float angleBetween = acos(clamp(dot, -1.0f, 1.0f));
	if (degrees(angleBetween) > 180.0f) {return -1e3f; /* Behind camera. */}

	float dotDegrees = (1.0f - dot) * 180.0f;

	float cross = spriteDirection.x * playerDirection.y - spriteDirection.y * playerDirection.x; //2D cross product
	int dotDirection = (cross >= 0.0f) ? 1 : -1;


	float screenXRelative = tan(angleBetween) / tan(radians(maxRayAngle));
	screenXRelative = (zoom) ? screenXRelative*zoomFactor : screenXRelative;
	float centrePixelX = (screenDimentions.x / 2.0f) + (dotDirection * screenXRelative * (screenDimentions.x / 2.0f));

	if (centrePixelX + (onScreenWidth/2) < 0 || centrePixelX - (onScreenWidth/2) > screenDimentions.x) {return -1e3f; /* Also offscreen, horizontally. */}

	return centrePixelX;
}



void main() {
	fragPosition = gl_FragCoord.xy;
	screenDimentions = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);	
	float fragDepth = imageLoad(renderedFrame, framePosition).a;

	vec2 closestUV = vec2(1e30f, 1e30f);
	Sprite closestSprite;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;


	for (int index = 0; index < 32; index++) {
		Sprite thisSprite = sprites[index];
		thisSprite.padding[0] = 0.0f; thisSprite.padding[1] = 0.0f;
		if (thisSprite.valid == 0) {continue; /* Sprite is empty */}

		float spriteDistance = length(playerPosition - thisSprite.position);

		if (spriteDistance > maxRayDistance || spriteDistance >= fragDepth) {continue; /* Too far to see onscreen. */}


		float spriteMaxHeight = 1.0f;

		float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (screenDimentions.x / screenDimentions.y));
		float viewAngleOffset = atan(spriteMaxHeight/spriteDistance);
		float spriteHeight = (screenDimentions.y * 2 * viewAngleOffset) / verticalFOV;

		float spriteWidth = thisSprite.width * spriteHeight;

		spriteWidth = (zoom) ? spriteWidth * zoomFactor : spriteWidth;
		spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;


		float centrePixelX = getSpriteScreenX(thisSprite, spriteWidth);
		if (centrePixelX < -(spriteWidth/2.0f) || centrePixelX >= screenDimentions.x + (spriteWidth/2.0f)) {continue; /* Offscreen, horizontally. */}


		vec2 spriteUV = getSpriteUV(thisSprite, centrePixelX, spriteDistance, vec2(spriteWidth, spriteHeight));
		if (spriteUV == vec2(1e30f, 1e30f)) {continue;}
		closestUV = spriteUV;
		closestSprite = thisSprite;
		fragDepth = spriteDistance;
	}



	if (closestUV == vec2(1e30f, 1e30f)) {return;}


	//Only write the final pixel to the frame.
	vec3 finalColour;
	if (drawUV == 1) {
		finalColour = vec3(closestUV.xy, closestSprite.textureID/16);
	} else {
		finalColour = texture(textureArray, vec3(closestUV.xy, float(closestSprite.textureID))).rgb;
	}
	vec4 finalFragColour = vec4(finalColour, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}