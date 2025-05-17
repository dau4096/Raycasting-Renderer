/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;

uniform ivec2 screenResolution;
uniform sampler2D renderedFrame;
uniform int smoothingEnabled;


void main() {
	vec2 UV;
	if (smoothingEnabled > 0) { //Simple Anti-Aliasing
		vec3 colourSum = vec3(0.0f, 0.0f, 0.0f);
		for (int x=-1; x<=1; x++) {
			for (int y=-1; y<=1; y++) {
				UV = (gl_FragCoord.xy + vec2(x, y)) / vec2(screenResolution);
				colourSum += texture(renderedFrame, UV).rgb;
			}
		}
		fragColour = vec4(colourSum / 9.0f, 1.0f);
	} else {
		UV = gl_FragCoord.xy / vec2(screenResolution);
		fragColour = vec4(texture(renderedFrame, UV).rgb, 1.0f);
	}
}