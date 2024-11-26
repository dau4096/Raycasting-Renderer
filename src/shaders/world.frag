//world.frag
#version 460 core

layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;
	float dimmingStrength;

	vec3 topColour;
	vec3 lowColour;

	int drawUV;
};

struct Wall {
	vec2 start;	       // Wall start point
	vec2 end;          // Wall end point
	int textureID;     // Wall texture Index
	int valid;         // Wall; Valid or not?
	float padding[2];  // Padding to align with 16B
};
layout(std140, binding=2) uniform wallUBO {
	Wall walls[128];
};

void main() {
	ivec2 fragPosition = ivec2(gl_FragCoord.xy);
	ivec2 screenDimentions = imageSize(renderedFrame);
	//vec4 fragColour = (fragPosition.y > (screenDimentions.y/2)) ? vec4(0.5294, 0.8078, 0.9216, maxRayDistance) : vec4(0.5000, 0.5000, 0.5000, maxRayDistance);
	vec4 fragColour = (fragPosition.y > (screenDimentions.y/2)) ? vec4(0.5294, 0.8078, 0.9216, maxRayDistance) : vec4(0.5000, 0.5000, 0.5000, maxRayDistance);

	imageStore(renderedFrame, fragPosition, fragColour);
}