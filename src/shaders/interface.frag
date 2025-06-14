/* interface.frag */
#version 460 core

layout(binding=0) uniform sampler2D renderedFrame;
layout(binding=1) uniform sampler2DArray textureArrayEnvironment;
layout(binding=2) uniform sampler2DArray textureArrayUI;
layout(binding=3) uniform sampler2DArray textureArrayNumeric;
layout(rgba32f, binding=0) uniform image2D interfaceTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float zoomFactor;
uniform bool zoom;
uniform ivec2 interfaceResolution;
uniform ivec2 renderResolution;
uniform ivec2 screenResolution;

//Player Data
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform vec4 screenTint;
uniform int health;
uniform int energy;

//Other
uniform int freq;
uniform bool showFreq;
uniform bool showData;
uniform int numTextObjects;


vec2 fragPosition, tiltedFragPosition;
ivec2 framePosition;
vec3 fragColour;
float fragDepth, chosenDepth = -1.0f;
float zoomEffect;

const float INF = 0xFFFFFF;




struct textArray{
	ivec4 contents[16];
	int length, scale;
};
struct TextObject {
	ivec4 text[16];	//Array of character indices.
	int length;		//Length of text.
	int scale;		//Text scale.
	int valid;		//TextObject validity.
	int _paddingA;	//Padding
	vec3 position;  //3D position.
	int centreX;	//Padding
};
layout(std430, binding=4) buffer textObjectSSBO {
	TextObject textObjects[];
};

textArray createTAFromTO(TextObject TO) {
	textArray TA;
	TA.contents = TO.text;
	TA.length = TO.length;
	TA.scale = TO.scale;
	return TA;
}

textArray createSingleCharTA(int c, int scale=1) {
	textArray TA;
	TA.contents[0].x = c;
	TA.length = 1;
	TA.scale = scale;
	return TA;
}



void drawRect(
		vec2 position, vec2 dimensions,
		vec3 rectColour, vec2 thisFragPos=fragPosition,
		bool overwritePrevious=false
	) {
	vec2 relativePos = position - thisFragPos;

	if (thisFragPos.x < position.x || thisFragPos.x >= position.x + dimensions.x ||
		thisFragPos.y < position.y || thisFragPos.y >= position.y + dimensions.y) {
		return;
	}

	if (overwritePrevious || (fragDepth != -1.0f)) {
		fragColour = rectColour;
		fragDepth = chosenDepth;
	}
}


void _renderImage(
		vec2 position, vec2 scale,
		int imageID, bool blendAlpha,
		sampler2DArray texArray,
		vec2 thisFragPos,
		bool outline, vec3 outlineColour,
		vec4 tintColour
	) {
	if (0 > imageID) {return; /* Invalid imageID */}
	vec2 relativePos = position - thisFragPos;

	if (thisFragPos.x < position.x || thisFragPos.x >= position.x + scale.x ||
		thisFragPos.y < position.y || thisFragPos.y >= position.y + scale.y) {
		return;
	}

	vec3 UV = vec3(
		-relativePos.x / scale.x,
		relativePos.y / scale.y,
		float(imageID)
	);

	vec4 albedo = texture(texArray, UV) * tintColour;
	if (outline) {
		drawRect(position, scale, outlineColour, thisFragPos);
	}
	if (blendAlpha) {
		fragColour = mix(fragColour, albedo.rgb, albedo.a);
		if (albedo.a > 0.0f) {
			fragDepth = chosenDepth;
		}
	} else {
		if (albedo.a >= 0.5f) {
			fragColour = albedo.rgb;
			fragDepth = chosenDepth;
		}
	}
}
//Wrappers.
void renderImage(
		vec2 position, vec2 scale,
		int imageID, bool blendAlpha=true,
		sampler2DArray texArray=textureArrayUI,
		vec2 thisFragPos=fragPosition,
		bool outline=false, vec3 outlineColour=vec3(0.0f, 0.0f, 0.0f),
		vec3 tintColour=vec3(1.0f, 1.0f, 1.0f)
	) {
	_renderImage(
		position, scale, imageID, blendAlpha,
		texArray, thisFragPos, outline, outlineColour,
		vec4(tintColour.rgb, 1.0f)
	);
}
void renderImage(
		vec2 position, vec2 scale,
		int imageID,
		vec3 tintColour, bool blendAlpha=true,
		sampler2DArray texArray=textureArrayUI,
		vec2 thisFragPos=fragPosition,
		bool outline=false, vec3 outlineColour=vec3(0.0f, 0.0f, 0.0f)
	) {
	_renderImage(
		position, scale, imageID, blendAlpha,
		texArray, thisFragPos, outline, outlineColour,
		vec4(tintColour.rgb, 1.0f)
	);
}




void drawInt(vec2 position, int scale, int value, vec3 tintColour=vec3(1.0f, 1.0f, 1.0f)) { //Values [-99999 <-> 99999] inclusive.
	int absVal = abs(value);
	int maxDigits = 5;
	bool started = false;
	int divisor = 10000;

	vec2 digitOffset = vec2(scale * 0.65f, 0.0);


	if (value < 0) {
		renderImage(position, vec2(scale), 10, tintColour, false, textureArrayNumeric); //"-"
		position += digitOffset;
	}

	for (int i = 0; i < maxDigits; ++i) {
		int digit = (absVal / divisor) % 10;

		if (digit > 0 || started || (i == maxDigits - 1)) {
			started = true;
			renderImage(position, vec2(scale), digit, tintColour, false, textureArrayNumeric); //"[0-9]"
			position += digitOffset;
		}

		divisor /= 10;
	}
}


void drawCrosshair() {
	const float radius = 7.5f;
	const float thickness = 2.0f;
	const vec4 crosshairColour = vec4(0.5f, 0.5f, 0.5f, 0.75f);

	vec2 centreScreen = interfaceResolution/2.0f;
	float dist = length(fragPosition - centreScreen) - radius;
	if ((dist > 0) && (dist < thickness)) {
		fragColour = mix(fragColour, crosshairColour.rgb, crosshairColour.a);
		fragDepth = -1.0f;
	}
}



void addVignetteShading() {
	vec3 tintShade = screenTint.rgb;
	float blending = screenTint.a;
	fragColour = mix(fragColour, tintShade, blending);
}


void _drawText(
		textArray TA, vec2 centre, float scale,
		bool hasBackground, vec3 backgroundColour,
		vec2 thisFragPos, vec3 tintColour
	) {
	for (int letterIdx=0; letterIdx<TA.length; letterIdx++) {
		//Iterate through letters.
		ivec4 charIvec4 = TA.contents[letterIdx / 4];
		int mod = letterIdx % 4;
		int charIdx = charIvec4[mod];


		if (charIdx == -1) {continue; /* Blank Character */}
		float charX = centre.x + scale*0.65f*(letterIdx - (TA.length/2.0f));
		vec2 charPos = vec2(charX, centre.y);


		if ((charIdx == -2) && hasBackground) {
			drawRect(charPos, vec2(scale, scale), backgroundColour, thisFragPos);
			continue;
		}

		renderImage(charPos, vec2(scale, scale), charIdx, true, textureArrayNumeric, thisFragPos, hasBackground, backgroundColour, tintColour);
	}
}
//Wrappers
void drawText(
		textArray TA, vec2 centre, float scale,
		bool hasBackground=false, vec3 backgroundColour=vec3(0.0f, 0.0f, 0.0f),
		vec2 thisFragPos=fragPosition, vec3 tintColour=vec3(1.0f, 1.0f, 1.0f)
	) {
	_drawText(
		TA, centre, scale,
		hasBackground, backgroundColour,
		thisFragPos, tintColour
	);
}
void drawText(
		textArray TA, vec2 centre, float scale, vec3 tintColour,
		bool hasBackground=false, vec3 backgroundColour=vec3(0.0f, 0.0f, 0.0f),
		vec2 thisFragPos=fragPosition
	) {
	_drawText(
		TA, centre, scale,
		hasBackground, backgroundColour,
		thisFragPos, tintColour
	);
}



void drawTextObjects(float rayAngle) {
	if (fragDepth < 0.0f) {return; /* UI Element here */}
	const float minDepthSQ = 0.5f;
	const bool hasBackground = true;
	const vec3 backgroundColour = vec3(0.0f, 0.0f, 0.0f);

	for (int idx=0; idx<numTextObjects; idx++) {
		TextObject thisTO = textObjects[idx];

		if ((thisTO.length > 0.0f) && (thisTO.scale >= 1.0f)) {
			vec2 delta = playerPosition.xy - thisTO.position.xy;
			float TODepthSQ = dot(delta, delta);
			if ((TODepthSQ >= (fragDepth*fragDepth)) || (TODepthSQ < minDepthSQ)) {continue; /* Obscured */}
			
			float invdistance = inversesqrt(TODepthSQ);
			chosenDepth = -1.0f / invdistance;
			float scale = thisTO.scale * invdistance * zoomEffect;

			//X
			if ((thisTO.centreX + scale * 0.65f * (thisTO.length / 2.0f) < 0.0f) || 
				(thisTO.centreX - scale * 0.65f * (thisTO.length / 2.0f) > interfaceResolution.x)) {
				continue; // Off-screen horizontally
			}

			//Y
			/*
			//Original from getWallUV() in environment.frag
			float projectedYTop = (originPos.z - wallTopZ) / distance;
			float screenYLow = renderResolution.y * (0.5 - projectedYLow);
			*/
			float projCentreY = (playerPosition.z - thisTO.position.z) * invdistance;
			float centreY = interfaceResolution.y * (0.5f - projCentreY);
			float charY = centreY - (scale / 2.0f);

			drawText(createTAFromTO(thisTO), vec2(thisTO.centreX, charY), scale, hasBackground, backgroundColour, tiltedFragPosition);
		}
	}
}



void main() {
	fragPosition = gl_FragCoord.xy;
	tiltedFragPosition = fragPosition;

	ivec2 framePosition = ivec2(gl_FragCoord.xy);
	vec4 frameColour = texture(renderedFrame, fragPosition/vec2(interfaceResolution));
	fragColour = frameColour.rgb;
	fragDepth = frameColour.a;

	zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f);
	tiltedFragPosition.y -= (tiltedFragPosition.x - interfaceResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f);
	tiltedFragPosition.y -= (pitchDecimal * renderResolution.y) / 72.9f; //Scaling to resolution. 10px per degree if it's 540px tall.


	addVignetteShading();


	drawTextObjects(rayAngle);


	//Show freq.
	if (showFreq) {
		drawInt(vec2(0, 368), 32, freq);
	}


	//Render stats.
	renderImage(vec2(-16, -72), vec2(192.0, 192.0), 0);
	drawInt(vec2(32, 32), 40, health);

	renderImage(vec2(460, -72), vec2(192, 192), 1);
	drawInt(vec2(520, 32), 40, energy);

	drawCrosshair();


	if (showData) {
		const vec3 dataTint = vec3(1.0f, 0.0f, 1.0f);
		drawInt(vec2(130, 368), 32, int(round(playerPosition.x)), dataTint);
		drawText(createSingleCharTA(47), vec2(125, 373), 20, dataTint); //X
		drawInt(vec2(210, 368), 32, int(round(playerPosition.y)), dataTint);
		drawText(createSingleCharTA(48), vec2(205, 373), 20, dataTint); //Y
		drawInt(vec2(290, 368), 32, int(round(playerPosition.z)), dataTint);
		drawText(createSingleCharTA(49), vec2(285, 373), 20, dataTint); //Z
		drawInt(vec2(440, 368), 32, int(round(playerViewAngle)), dataTint);
		drawText(createSingleCharTA(24), vec2(435, 373), 20, dataTint); //A
	}


	vec4 finalFragColour = vec4(fragColour.rgb, (fragDepth < 0.0f) ? 1.0f : 0.0f);
	imageStore(interfaceTexture, framePosition, finalFragColour);
}