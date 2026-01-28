/* crt.frag */

#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;


layout(binding=0) uniform sampler2D renderedFrameSampler2D;
layout(binding=1) uniform sampler2D depthMap;
layout(binding=2) uniform sampler2D interfaceTexture;
layout(binding=3) uniform sampler2DArray lightMapsArray;
layout(binding=4) uniform sampler2D positionMap;

layout(rgba32f, binding=0) writeonly uniform image2D frameToScreenshot;


//Camera
uniform float maxRayDistance;
uniform ivec2 screenResolution;
uniform ivec2 renderResolution;

//Sky
uniform vec3 fogColour;

//Debug
uniform int debugMode;

//Other
uniform bool antiAliasing;
uniform int quantisingLevel;
uniform int lightingType;
uniform bool screenshotHasHUD;
uniform bool shouldTakeScreenshot;
uniform uint numLights;
uniform vec4 screenTint;
uniform bool isInvertEffect;


#define EPSILON 1e-4f

//////////////// Config stuff ////////////////
//CRT;
#define SCREEN_CURVATURE 5.0f
#define SCREEN_NORMALS false
#define BEZEL_UV_BOUNDARY_LO 0.01f
#define BEZEL_UV_BOUNDARY_HI 1.0f - BEZEL_UV_BOUNDARY_LO
#define BEZEL_NORMAL_STRENGTH 1.0f
#define SCALING 1.0f

//Lighting;
#define MIN_BRIGHTNESS 0.175f
#define MAX_BRIGHTNESS 2.25f
#define NUM_PSEUDO_LIGHTS 2
//////////////// Config stuff ////////////////



vec2 getUV(vec2 pos) {
	return vec2(pos) / vec2(screenResolution);
}


vec2 curveOffset;
vec2 curveRemap(vec2 uv) {
	//Found online: [https://github.com/swiftcoder/fathom/blob/cd56fce9528641c7ed177da70876bf478d90b4fa/src/post_processor.rs#L231-L239]
	uv = uv * 2.0f - 1.0f;
	curveOffset = abs(uv.yx) / vec2(SCREEN_CURVATURE, SCREEN_CURVATURE);
	uv = uv + uv * curveOffset * curveOffset;
	uv = uv * 0.5f + 0.5f;
	return uv;
}


uint pcg_hash(uint seed) {
	/*
	Hash function taken from;
	https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
	*/
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}


void main() {
	vec2 mainUV = getUV(gl_FragCoord.xy);

	vec2 newUV = (mainUV * 1.25f) - vec2(0.125f, 0.25f);
	bool inBezel = (
		((newUV.x >= 0.0f) && (newUV.x <= 1.0f)) &&
		((newUV.y >= 0.0f) && (newUV.y <= 1.0f))
	);

	vec2 curvedUV = curveRemap(newUV);
	bool inScreen = (
		((curvedUV.x > BEZEL_UV_BOUNDARY_LO) && (curvedUV.x < BEZEL_UV_BOUNDARY_HI)) &&
		((curvedUV.y > BEZEL_UV_BOUNDARY_LO) && (curvedUV.y < BEZEL_UV_BOUNDARY_HI))
	);

	vec3 normalDirection = vec3(0.0f, 0.0f, 1.0f);
	if (inBezel && !inScreen) {
		if (curvedUV.x < BEZEL_UV_BOUNDARY_LO) {normalDirection.x = BEZEL_NORMAL_STRENGTH;}
		if (curvedUV.x > BEZEL_UV_BOUNDARY_HI) {normalDirection.x = -BEZEL_NORMAL_STRENGTH;}
		if (curvedUV.y < BEZEL_UV_BOUNDARY_LO) {normalDirection.y = BEZEL_NORMAL_STRENGTH;}
		if (curvedUV.y > BEZEL_UV_BOUNDARY_HI) {normalDirection.y = -BEZEL_NORMAL_STRENGTH;}
		normalDirection = normalize(normalDirection);
	
	}

	if (inScreen && SCREEN_NORMALS) {
		normalDirection.xy = curveOffset;
	} else {
		uint seedX = uint(mainUV.x * SCALING) * 73856093u ^ uint(mainUV.y * SCALING) * 19349663u;
		uint seedY = uint(mainUV.y * SCALING) * 73856093u ^ uint(mainUV.x * SCALING) * 19349663u;
		/*
		normalDirection.xy += vec2(
			(pcg_hash(seedX) & 0xFF) - 0x7F,
			(pcg_hash(seedY) & 0xFF) - 0x7F
		) / 512.0f;
		*/
	}


	//fragColour = vec4(normalDirection.xyz * 0.5f + 0.5f, 1.0f); return; //Normal map
	fragColour = (inScreen) ? vec4(curvedUV.xy, 1.0f, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f); return; //UV;

	fragColour = (inScreen) ? vec4(texture(renderedFrameSampler2D, curvedUV).rgb, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f); //
}