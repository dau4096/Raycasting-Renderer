/* interface.frag */
#version 460 core

layout(binding=0) uniform sampler2DArray textureArrayUI;
layout(binding=1) uniform sampler2DArray textureArrayNumeric;
layout(binding=2) uniform sampler2D depthMap;

layout(location=0) out vec4 outFragColour;


in vec3 fragUV;
in vec3 fragPos;
in flat float fragDistance;


uniform ivec2 interfaceResolution;
uniform float zoomFactor;
uniform bool zoom;
uniform float playerViewRoll;
uniform float maxRayDistance;


void main() {
	vec2 fragPosition = gl_FragCoord.xy;

	bool isTextObject = fragDistance > 0.0f;
	if (isTextObject) {
		//TextObjects have roll and pitch applied, pitch being done during VAO data creation.
		//Both use the same calculation as environment.frag and sprites.frag
		float zoomEffect = (zoom) ? zoomFactor : 1.0f;
		//Negative is upward; so subtract.
		float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
		fragPosition.y += (fragPosition.x - interfaceResolution.x / 2.0f) * rollDecimal;
	}

	if (fragDistance >= texture(depthMap, fragPosition / vec2(interfaceResolution)).r * maxRayDistance) {discard;}
	ivec2 framePosition = ivec2(fragPosition);

	vec4 fragColour;
	int intZ = int(floor(fragUV.z + 0.5f));
	int texID = intZ >> 2;
	bool alphaNumeric = (intZ & 1) > 0;
	bool isBlackfill = (intZ & 2) > 0;
	if (alphaNumeric) { //Alphanumeric values
		fragColour = texture(textureArrayNumeric, vec3(fragUV.xy, texID));
	} else {
		fragColour = texture(textureArrayUI, vec3(fragUV.xy, texID));
	}

	if (fragColour.a < 0.5f) {discard;}
	if (isTextObject) {fragColour.a = 2.0f;}
	outFragColour = fragColour;
}