/* displacements.frag */
#version 460 core

layout(binding=0) uniform sampler2D renderedFrameRO;
layout(binding=1) uniform sampler2D FBOColour;
layout(binding=2) uniform sampler2D FBOPosition;
layout(binding=3) uniform sampler2D FBONormal;
layout(rgba32f, binding=0) writeonly uniform image2D renderedFrameWO;
layout(rgba32f, binding=1) writeonly uniform image2D positionMap;
layout(rgba32f, binding=2) writeonly uniform image2D normalMap;



uniform float shadowMapQuality;
uniform ivec2 renderResolution;
uniform float maxRayDistance;

#define EPSILON 1e-4f



void main() {
	vec2 fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);

	vec2 UV = fragPosition / vec2(renderResolution);
	float fragDepth = texture(renderedFrameRO, UV).w;
	vec4 albedo = texture(FBOColour, UV);
	if ((albedo.w > fragDepth) || (albedo.w >= maxRayDistance - EPSILON)) {return; /* Displacement frag is obscured. */}
	vec4 normal = texture(FBONormal, UV);
	if (normal.w > 0) { //Disp was hit.
		imageStore(renderedFrameWO, framePosition, albedo);
		if (shouldDrawToPositionMap) {
			vec4 pos = texture(FBOPosition, UV);
			uint idx = (int(pos.w) << 3) | 0x3;
			ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
			imageStore(positionMap, thisFramePosition, vec4(pos.xyz, float(idx)));
			imageStore(normalMap, thisFramePosition, vec4(normal.xyz, 1.0f));
		}
	}
}