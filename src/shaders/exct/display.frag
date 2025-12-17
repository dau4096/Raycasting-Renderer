/* display.frag */
#version 460 core

in vec2 fragUV;
out vec4 fragColour;

layout(location=0) uniform sampler2D finishedFrame;
//layout(location=1) uniform sampler2D finishedInterface;

void main() {

	fragColour = texture(finishedFrame, fragUV.xy);

}