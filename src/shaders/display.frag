/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;

uniform ivec2 screenResolution;
ivec2 renderResolution;

layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 10) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;

	vec2 textureSize;

	float padding[4];
};


void main() {
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(
		floor(gl_FragCoord.x * float(renderResolution.x) / float(screenResolution.x)),
		floor(gl_FragCoord.y * float(renderResolution.y) / float(screenResolution.y))
	);
	fragColour = vec4(imageLoad(renderedFrame, framePosition).rgb, 1.0f);
}