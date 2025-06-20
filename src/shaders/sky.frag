/* environment.frag */
#version 460 core


out vec4 fragColour;

//Samplers
layout(binding=0) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float zoomFactor;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewPitch;

//Debug
uniform int debugMode;

//Other
uniform ivec2 screenResolution;



const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.175f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);



vec4 fetchColour() {
	vec2 fragPosition = gl_FragCoord.xy;
	float halfFOV = maxRayDistance / 2.0f;
	float rayOffset = -halfFOV + (fragPosition.x / float(screenResolution.x)) * 2.0f * halfFOV;
	float rayAngleYaw = radians(playerViewAngle + rayOffset);
	float normY = (2.0 * fragPosition.y / screenResolution.y) - 1.0;

	vec2 UV = vec2(
		fract(rayAngleYaw / 6.28318530718f), //Over 2*Pi.
		1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
	);


	if (debugMode == 1) {
		return vec4(UV.xy, 0.0f, 1.0f);
	}
	return texture(skyboxTexture, UV);
}




void main() {
	fragColour = fetchColour();
}