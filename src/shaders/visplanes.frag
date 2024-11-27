#version 460 core

uniform int topIndex; // Index for ceiling texture
uniform int lowIndex; // Index for floor texture
uniform bool zoom;
uniform float playerViewAngle;
uniform vec2 playerPosition;
uniform sampler2DArray textureArray;

layout(rgba32f, binding=0) uniform image2D renderedFrame;

layout(std140, binding = 1) uniform constUBO {
	float zoomFactor;	   // Zoom-in effect
	float maxRayAngle;	  // Maximum field of view angle
	float maxRayDistance;   // Maximum visible distance
	float dimmingStrength;  // Not used here, but reserved
	float toRad;
	vec2 textureSize;	   // Size of the texture
	int drawUV;			 // Debugging toggle
};


float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}


vec3 getUVCoords(vec2 fragPosition, ivec2 screenDimentions) {
	float normY = (2.0 * fragPosition.y / screenDimentions.y) - 1.0; // Normalized screen Y [-1, 1]
	float actualDistance = maxRayDistance / (1.0 + abs(normY) * zoomFactor); // Perspective scaling


	float offset = -maxRayAngle + (fragPosition.x / screenDimentions.x) * 2 * maxRayAngle; //0 being screen centre collumn, -/+ maxRayAngle at the left and right edge respectively.
	float angle = angleClamp(playerViewAngle + offset); //Actual angle, taking into account player view angle.
	vec2 direction = normalize(vec2(sin(angle * toRad), cos(angle * toRad))); //Direction vector from said angle.
	vec2 realPosition = playerPosition + (direction * actualDistance); //position ahead of the player, at the distance calculated from screen Y. (centre is maxRayDistance, top/bottom are both 0. Linear.)


	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = mod(realPosition.x, 5.0f) / 5.0f;
	float yUV = mod(realPosition.y, 5.0f) / 5.0f;
	//Texture index depends on top (ceiling) or bottom (floor) half.
	float index = (fragPosition.y < screenDimentions.y/2) ? topIndex : lowIndex;

	return vec3(xUV, yUV, index);
}


void main() {
	vec2 fragPosition = gl_FragCoord.xy;
	ivec2 screenDimentions = imageSize(renderedFrame);

	vec3 UVcoords = getUVCoords(fragPosition, screenDimentions);

	vec4 fragColour = texture(textureArray, UVcoords);

	//Debug visualise UV coordinates, and index (divided by 2 as both 1 and 2 would appear pure blue otherwise)
	//vec4 fragColour = vec4(UVcoords.xy, UVcoords.z/2, 1.0); // Visualize UV coords

	//Write the colour to the frame.
	ivec2 framePosition = ivec2(fragPosition);
	imageStore(renderedFrame, framePosition, fragColour);
}
