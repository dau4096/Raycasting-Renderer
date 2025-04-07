/* interface.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec3 playerPosition;
uniform bool zoom;
uniform vec4 screenTint;

vec2 fragPosition;
ivec2 renderResolution, framePosition;
vec3 fragColour;
float fragDepth;
float fragDepths[1920];


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 10) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;

	vec2 textureSize;

	float padding[4];
};

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std430, binding = 2) buffer visplaneUBO {
	Visplane visplanes[64];
};

struct Wall {
	vec3 start;			//Wall Start.
	vec3 end;			//Wall End.
	int textureID;		//Wall Texture.
	int valid;			//Wall Validity.
	float _padding[2];	//Wall Padding.
};
layout(std430, binding = 3) buffer wallUBO {
	Wall walls[256];
};

struct Sprite {
	vec2 position;	//Sprite Position.
	float width;	//Sprite Width.
	int textureID;	//Sprite Texture ID.
	int valid;		//Sprite Validity.
	float _padding;	//Memory padding.
};
layout(std140, binding = 4) uniform spriteSSBO {
	Sprite sprites[32];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 5) uniform lightUBO {
	Light lights[64];
};



vec3 addVignetteShading() {
	vec3 tintShade = screenTint.rgb;
	float blending = screenTint.a;
	return mix(fragColour, tintShade, blending);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);
	vec4 imageColour = imageLoad(renderedFrame, framePosition);
	fragColour = imageColour.rgb;
	fragDepth = imageColour.a;

	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	fragColour = addVignetteShading();






	vec4 finalFragColour = vec4(fragColour.rgb, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}