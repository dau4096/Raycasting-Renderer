/* crt.frag */

#version 460 core

in vec2 fragUV;
out vec4 fragColour;

layout(location=0) uniform sampler2D finishedFrame;
layout(location=1) uniform sampler2D finishedInterface;

void main() {

	vec4 frameColour = texture(finishedFrame, fragUV.xy);
	vec4 uiColour = texture(finishedInterface, fragUV.xy);
	fragColour = vec4(mix(frameColour.rgb, uiColour.rgb, uiColour.a).rgb, 1.0f);

}