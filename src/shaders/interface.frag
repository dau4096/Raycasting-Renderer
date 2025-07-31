/* interface.frag */
#version 460 core

layout(binding=0) uniform sampler2DArray textureArrayUI;
layout(binding=1) uniform sampler2DArray textureArrayNumeric;
layout(binding=2) uniform sampler2D renderedFrame;
layout(rgba32f, binding=0) writeonly uniform image2D interfaceTexture;


in vec3 fragUV;
in vec3 fragPos;
in flat float fragDistance;


uniform ivec2 interfaceResolution;
uniform float zoomFactor;
uniform bool zoom;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform float maxRayAngle;


void main() {
	vec2 fragPosition = gl_FragCoord.xy;

	if (fragDistance > 0.0f) {
		//TextObjects have roll and pitch applied, pitch being done during VAO data creation.
		//Both use the same calculation as environment.frag and sprites.frag
		float zoomEffect = (zoom) ? zoomFactor : 1.0f;
		//Negative is upward; so subtract.
		float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
		fragPosition.y += (fragPosition.x - interfaceResolution.x / 2.0f) * rollDecimal;
	}
	if (fragDistance >= texture(renderedFrame, fragPosition / vec2(interfaceResolution)).a) {discard;}
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
	imageStore(interfaceTexture, framePosition, fragColour);
}