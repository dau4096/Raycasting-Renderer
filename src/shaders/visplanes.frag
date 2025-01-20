/* visplanes.frag */
#version 460 core

uniform bool zoom;
uniform float playerViewAngle;
uniform vec2 playerPosition;
uniform sampler2DArray textureArray;
uniform int drawUV;


layout(rgba32f, binding=0) uniform image2D renderedFrame;

layout(std140, binding = 1) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;

	float topIndex;
	float lowIndex;

	vec2 textureSize;

	float padding[3];
};

layout(std430, binding = 5) buffer depthBuffer {
	float depths[];
};

struct Light {
	vec3 position;		//Light Position
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float padding[2];	//Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[32];
};



vec2 fragPosition;
ivec2 screenDimentions;
vec4 fragColour;
float actualDistance;

float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}



vec3 getUVCoords() {
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	bool topHalf = fragPosition.y > screenDimentions.y/2;
	float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (screenDimentions.x / screenDimentions.y));

	float ceilingHeight = 1.0f;

	float normY = (2.0 * fragPosition.y / screenDimentions.y) - 1.0; // Normalized screen Y [-1, 1]
	float viewAngleOffset = normY * (verticalFOV/2);
	float linearDistance = ceilingHeight / abs(tan(viewAngleOffset));
	actualDistance = clamp(linearDistance, 0.0f, maxRayDistance);



	float offset = -rayAngle + (fragPosition.x / screenDimentions.x) * 2 * rayAngle; //0 being screen centre collumn, -/+ maxRayAngle at the left and right edge respectively.
	float angle = angleClamp(playerViewAngle + offset); //Actual angle, taking into account player view angle.
	vec2 direction = normalize(vec2(sin(radians(angle)), cos(radians(angle)))); //Direction vector from said angle.
	vec2 realPosition = playerPosition + (direction * actualDistance); //position ahead of the player, at the distance calculated from screen Y. (centre is maxRayDistance, top/bottom are both 0. Linear.)


	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV;
	if (!topHalf) {
		xUV = 1.0f - fract(abs(realPosition.x));
	} else {
		xUV = fract(abs(realPosition.x));
	}
	float yUV = fract(abs(realPosition.y));
	//Texture index depends on top (ceiling) or bottom (floor) half.
	float index = (topHalf) ? topIndex : lowIndex;

	return vec3(xUV, yUV, index);
}


void main() {
	fragPosition = gl_FragCoord.xy;
	screenDimentions = imageSize(renderedFrame);

	vec3 UVcoords = getUVCoords();

	if (drawUV == 1) {
		fragColour = vec4(UVcoords.xy, UVcoords.z/2, 1.0); // Visualize UV coords
	} else {
		float distanceFade = 1.0f - (actualDistance / maxRayDistance);
		fragColour = texture(textureArray, UVcoords) * distanceFade; //Draw texture colour.
	}

	//Write the colour to the frame.
	ivec2 framePosition = ivec2(fragPosition);
	vec4 finalFragColour = vec4(fragColour.rgb, maxRayDistance);
	depths[framePosition.x] = maxRayDistance;
	imageStore(renderedFrame, framePosition, finalFragColour);
}
