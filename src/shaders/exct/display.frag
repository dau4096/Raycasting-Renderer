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
//Lighting;
#define MIN_BRIGHTNESS 0.175f
#define MAX_BRIGHTNESS 2.25f
#define NUM_PSEUDO_LIGHTS 2
//////////////// Config stuff ////////////////



vec2 getUV(vec2 pos) {
	return vec2(pos) / vec2(screenResolution);
}

float CURVATURE = 3.0f;

vec2 curveRemap(vec2 uv) {
	uv = uv * 2.0f - 1.0f;
	vec2 offset = abs(uv.yx) / vec2(CURVATURE, CURVATURE);
	uv = uv + uv * offset * offset;
	uv = uv * 0.5f + 0.5f;
	return uv;
}


void main() {
	vec2 mainUV = getUV(gl_FragCoord.xy);

	vec2 newUV = (mainUV * 1.5f) - vec2(0.25f, 0.4f);
	newUV = curveRemap(newUV);
	bool inRange = (
		((newUV.x > 0.01f) && (newUV.x < 0.99f)) &&
		((newUV.y > 0.01f) && (newUV.y < 0.99f))
	);

	fragColour = (inRange) ? vec4(texture(renderedFrameSampler2D, newUV).rgb, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f);
	//fragColour = (inRange) ? vec4(1.0f, 0.0f, 1.0f, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f);
}