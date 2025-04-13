/* interface.frag */
#version 460 core


layout(binding = 0) uniform sampler2DArray textureArrayEnvironment;
layout(binding = 1) uniform sampler2DArray textureArrayUI;
layout(binding = 2) uniform sampler2DArray textureArrayNumeric;

uniform float playerViewAngle;
uniform vec3 playerPosition;
uniform bool zoom;
uniform vec4 screenTint;
uniform int health;
uniform int energy;
uniform ivec2 screenResolution;
uniform int FPS;
uniform int showFreq;

vec2 fragPosition;
ivec2 renderResolution, framePosition;
vec3 fragColour;
float fragDepth;


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




void renderImage(vec2 position, vec2 scale, int imageID, bool blendAlpha=true, sampler2DArray texArray=textureArrayUI) {
	if ((0 > imageID) || (imageID > 32)) {return; /* Invalid imageID */}
	if ((fragPosition.x < position.x) || (fragPosition.y < position.y)) {return;}
	vec2 relativePos = position - fragPosition;

	if (fragPosition.x < position.x || fragPosition.x >= position.x + scale.x ||
		fragPosition.y < position.y || fragPosition.y >= position.y + scale.y) {
		return;
	}

	vec3 UV = vec3(
		-relativePos.x / scale.x,
		relativePos.y / scale.y,
		float(imageID)
	);

	vec4 albedo = texture(texArray, UV);
	if (blendAlpha) {
		fragColour = mix(fragColour, albedo.rgb, albedo.a);
	} else {
		fragColour = albedo.rgb;
	}
}



void drawInt(vec2 position, int scale, int value) { //Values [-99999 <-> 99999] inclusive.
	int absVal = abs(value);
	int maxDigits = 5;
	bool started = false;
	int divisor = 10000;

	vec2 digitOffset = vec2(scale, 0.0);


	if (value < 0) {
		renderImage(position, vec2(scale), 10, true, textureArrayNumeric); //"-"
		position += digitOffset;
	}

	for (int i = 0; i < maxDigits; ++i) {
		int digit = (absVal / divisor) % 10;

		if (digit > 0 || started || (i == maxDigits - 1)) {
			started = true;
			renderImage(position, vec2(scale), digit, true, textureArrayNumeric); //"[0-9]"
			position += digitOffset;
		}

		divisor /= 10;
	}
}


void drawCrosshair() {
	const int radius = 5;
	const int thickness = 1;
	const vec4 crosshairColour = vec4(0.25f, 0.25f, 0.25f, 0.5f);

	ivec2 centreScreen = renderResolution/2;
	float dist = length(fragPosition - centreScreen) - radius;
	if ((dist > 0) && (dist < thickness)) {
		fragColour = mix(fragColour, crosshairColour.rgb, crosshairColour.a);
	}
}



void addVignetteShading() {
	vec3 tintShade = screenTint.rgb;
	float blending = screenTint.a;
	fragColour = mix(fragColour, tintShade, blending);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);
	vec4 imageColour = imageLoad(renderedFrame, framePosition);
	fragColour = imageColour.rgb;
	fragDepth = imageColour.a;

	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;



	addVignetteShading();


	//Show FPS.
	if (showFreq > 0) {
		drawInt(vec2(0, 245), 25, FPS);
	}


	//Render stats.
	renderImage(vec2(-16, -48), vec2(128, 128), 0);
	drawInt(vec2(8, 24), 25, health);

	renderImage(vec2(360, -48), vec2(128, 128), 1);
	drawInt(vec2(400, 24), 25, energy);


	drawCrosshair();



	vec4 finalFragColour = vec4(fragColour.rgb, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}