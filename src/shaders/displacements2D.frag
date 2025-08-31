/* displacements.frag */
#version 460 core

layout(binding=0) uniform sampler2D depthMap;
layout(binding=1) uniform sampler2D FBOColour;
layout(binding=2) uniform sampler2D FBOPosition;
layout(binding=3) uniform sampler2D FBONormal;

//FBO components
layout(location=0) out vec4 outFragColour;
layout(location=1) out vec4 outFragPosition;
layout(location=2) out vec4 outFragNormal;


uniform float shadowMapQuality;
uniform ivec2 renderResolution;
uniform float maxRayDistance;

#define EPSILON 1e-4f



void main() {
	vec2 fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);

	vec2 UV = fragPosition / vec2(renderResolution);
	float fragDepth = texture(depthMap, UV).w * maxRayDistance;

	vec4 albedo = texture(FBOColour, UV);
	float dispDepth = albedo.r * maxRayDistance;
	if ((dispDepth > fragDepth) || (dispDepth >= maxRayDistance - EPSILON)) {discard; /* Displacement frag is obscured. */}

	vec4 normal = texture(FBONormal, UV);
	if (normal.w <= 0) {discard; /* Disp was not hit. */}

	outFragColour = vec4(albedo.rgb, 1.0f);
	gl_FragDepth = dispDepth;

	if (shouldDrawToPositionMap) {
		vec4 pos = texture(FBOPosition, UV);
		uint idx = (int(pos.w) << 3) | 0x3;
		ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
		outFragPosition = vec4(pos.xyz, float(idx));
		outFragNormal = vec4(normal.xyz, 1.0f);
	}
}