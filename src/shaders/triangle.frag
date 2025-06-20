/* environment.frag */
#version 460 core


out vec4 fragColour;
in vec3 fragUV;
in vec3 fragPos;
in flat int fragType;



//Samplers
layout(binding=0) uniform sampler2DArray textureArray;

//CameraData
uniform vec2 textureScale;
uniform vec3 textureOffset;
uniform bool useMipMapping;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewPitch;

//Debug
uniform int debugMode;



const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.175f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);



vec4 fetchColour(vec3 UV) {
	if (debugMode == 1) {
		return vec4(UV.xy, UV.z / 32.0f, 1.0f);
	}

	if (fragType == 1) { //Visplanes
		UV.x = fract(fragPos.x / textureScale.x) + textureOffset.x;
		UV.y = fract(fragPos.y / textureScale.y) + textureOffset.y;
	} else if (fragType == 2) { //Wall with greater X delta
		UV.x =  fract(fragPos.x / textureScale.x) + textureOffset.x;
		UV.y = -fract(fragPos.z / textureScale.y) + textureOffset.z;
	} else if (fragType == 3) { //Wall with greater Y delta
		UV.x =  fract(fragPos.y / textureScale.x) + textureOffset.y;
		UV.y = -fract(fragPos.z / textureScale.y) + textureOffset.z;
	} else if (fragType == 5) {
		UV.y *= -1.0f;
	}

	if (useMipMapping) {
		return texture(textureArray, UV);
	}
	return textureLod(textureArray, UV, 0.0f);
}




void main() {
	vec4 albedo = fetchColour(fragUV);
	if (albedo.a < 0.5f) {
		discard;
	}
	fragColour.rgb = albedo.rgb;
}