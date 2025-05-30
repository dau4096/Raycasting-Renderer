/* interface.frag */
#version 460 core


layout(binding = 0) uniform sampler2DArray textureArrayEnvironment;
layout(binding = 1) uniform sampler2DArray textureArrayUI;
layout(binding = 2) uniform sampler2DArray textureArrayNumeric;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float zoomFactor;
uniform bool zoom;

//Player Data
uniform int itemTextureIDX;
uniform float itemTextOpacity;
uniform int itemTextLength;
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform vec4 screenTint;
uniform int health;
uniform int energy;

//Other
uniform ivec2 screenResolution;
uniform int freq;
uniform int showFreq;

vec2 fragPosition, tiltedFragPosition;
ivec2 renderResolution, framePosition;
vec3 fragColour;
float fragDepth;
float zoomEffect;
vec2 uiScaleFactor;

const ivec2 uiResolution = ivec2(480, 270);
const float INF = 0xFFFFFF;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;

struct textArray{
	ivec4 contents[16];
	int length;
	float opacity;
};
struct TextObject {
	ivec4 text[16];	//Array of character indices.
	int length;		//Length of text.
	int scale;		//Text scale.
	int valid;		//TextObject validity.
	int _paddingA;	//Padding
	vec3 position;  //3D position.
	float _paddingB;//Padding
};
layout(std140, binding=6) uniform textObjectUBO {
	TextObject textObjects[32];
};

textArray createTAFromTO(TextObject TO) {
	textArray TA;
	TA.contents = TO.text;
	TA.length = TO.length;
	TA.opacity = 1.0f;
	return TA;
}

layout(std140, binding=7) uniform itemTextBuffer {
	ivec4 itemText[16];
};



vec2 scaleUI(vec2 position) {
	return position * uiScaleFactor;
}

float scaleUI(float value) {
	return value * uiScaleFactor.x; // or .y if consistent scaling is needed
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
		fragDepth = -1.0f;
	}
}

void renderImage(
		vec2 position, vec2 scale,
		int imageID, bool blendAlpha=true,
		sampler2DArray texArray=textureArrayUI,
		vec2 thisFragPos=fragPosition,
		bool outline=false, vec3 outlineColour=vec3(0.0f, 0.0f, 0.0f),
		float opacity=1.0f
	) {
	if ((0 > imageID) /*|| (imageID > 32)*/) {return; /* Invalid imageID */}
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

	vec4 albedo = texture(texArray, UV);
	if (outline) {
		drawRect(position, scale, outlineColour, thisFragPos);
	}
	if (blendAlpha) {
		fragColour = mix(fragColour, albedo.rgb, albedo.a * opacity);
		if (albedo.a > 0.0f) {
			fragDepth = -1.0f;
		}
	} else {
		if (albedo.a >= 0.5f) {
			fragColour = mix(fragColour, albedo.rgb, opacity);
			fragDepth = -1.0f;
		}
	}
}



void drawInt(vec2 position, int scale, int value) { //Values [-99999 <-> 99999] inclusive.
	int absVal = abs(value);
	int maxDigits = 5;
	bool started = false;
	int divisor = 10000;

	vec2 digitOffset = vec2(scale * 0.65f, 0.0);


	if (value < 0) {
		renderImage(position, vec2(scale), 10, false, textureArrayNumeric); //"-"
		position += digitOffset;
	}

	for (int i = 0; i < maxDigits; ++i) {
		int digit = (absVal / divisor) % 10;

		if (digit > 0 || started || (i == maxDigits - 1)) {
			started = true;
			renderImage(position, vec2(scale), digit, false, textureArrayNumeric); //"[0-9]"
			position += digitOffset;
		}

		divisor /= 10;
	}
}


void drawCrosshair() {
	float radius = scaleUI(5.0);
	float thickness = scaleUI(1.0);
	const vec4 crosshairColour = vec4(0.5f, 0.5f, 0.5f, 0.75f);

	vec2 centreScreen = renderResolution/2.0f;
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


void drawText(textArray TA, vec2 centre, float scale, bool hasBackground=false, vec3 backgroundColour=vec3(0.0f, 0.0f, 0.0f), vec2 thisFragPos=fragPosition) {
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

		renderImage(charPos, vec2(scale, scale), charIdx, true, textureArrayNumeric, thisFragPos, hasBackground, backgroundColour, TA.opacity);
	}
}



//TextObject stuff.
float getTOScreenX(TextObject thisTO, float rayAngle) {
	float f = tan(radians(rayAngle)); //tan(FOV/2)
	float a = radians(playerViewAngle);

	vec2 dir = vec2(sin(a), cos(a));
	vec2 plane = vec2(-cos(a) * f, sin(a) * f);
	vec2 TODir = thisTO.position.xy - playerPosition.xy;

	if (dot(dir, normalize(TODir)) < 0.0f) {return INF;}


	float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

	float transformX = invDet * (dir.y * TODir.x - dir.x * TODir.y);
	float transformY = invDet * (-plane.y * TODir.x + plane.x * TODir.y);

	return (renderResolution.x / 2.0f) * (1.0f - transformX / transformY);
}

void drawTextObjects(float rayAngle) {
	if (fragDepth < 0.0f) {return; /* UI Element here */}
	const float minDepthSQ = 1.0f;
	const bool hasBackground = true;
	const vec3 backgroundColour = vec3(0.0f, 0.0f, 0.0f);

	for (int idx=0; idx<32; idx++) {
		TextObject thisTO = textObjects[idx];
		if (thisTO.valid <= 0) {break; /* End of valid TextObjects */}
		if ((thisTO.length > 0.0f) && (thisTO.scale >= 1.0f)) {
			vec3 delta = playerPosition - thisTO.position;
			float TODepthSQ = dot(delta, delta);
			if ((TODepthSQ >= (fragDepth*fragDepth)) || (TODepthSQ < minDepthSQ)) {continue; /* Obscured */}
			
			float invdistance = inversesqrt(TODepthSQ);
			float scale = thisTO.scale * invdistance * zoomEffect;
			if (scale < 1.0f) {continue; /* Scale too small to see. */}

			//X
			float centreX = getTOScreenX(thisTO, rayAngle);
			if ((centreX + scale * 0.65f * (thisTO.length / 2.0f) < 0.0f) || 
				(centreX - scale * 0.65f * (thisTO.length / 2.0f) > renderResolution.x)) {
				continue; //Off-screen horizontally
			}

			//Y
			float verticalRatio = (playerPosition.z - thisTO.position.z) * invdistance;
			float centreY = (renderResolution.y / 2.0f) - verticalRatio * renderResolution.y * zoomEffect;
			float charY = centreY - (scale / 2.0f);

			drawText(createTAFromTO(thisTO), vec2(centreX, charY), scale, hasBackground, backgroundColour, tiltedFragPosition);
		}
	}
}



void drawHeldItem() {
	//Show item visual.
	renderImage(vec2((renderResolution.x/2.0f)-96.0f, -32.0f), vec2(192, 192), itemTextureIDX);


	//Top of screen, item name.
	textArray titleText;
	titleText.contents = itemText;
	titleText.length = itemTextLength;
	titleText.opacity = itemTextOpacity;

	drawText(titleText, vec2(renderResolution.x/2.0f, renderResolution.y-50), 50);
}



void main() {
	renderResolution = imageSize(renderedFrame);
	uiScaleFactor = vec2(renderResolution) / vec2(uiResolution);
	fragPosition = gl_FragCoord.xy;
	tiltedFragPosition = fragPosition;
	ivec2 framePosition = ivec2(gl_FragCoord.xy);
	vec4 imageColour = imageLoad(renderedFrame, framePosition);
	fragColour = imageColour.rgb;
	fragDepth = imageColour.a;

	zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f);
	tiltedFragPosition.y -= (tiltedFragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f);
	tiltedFragPosition.y -= pitchDecimal * 10.0f; //10x scaling.


	addVignetteShading();


	drawTextObjects(rayAngle);


	//Show freq.
	if (showFreq > 0) {
		drawInt(scaleUI(vec2(0, 245)), int(scaleUI(25.0)), freq);
	}


	//Render stats.
	renderImage(scaleUI(vec2(-16, -48)), scaleUI(vec2(128.0, 128.0)), 0);
	drawInt(scaleUI(vec2(16, 24)), int(scaleUI(25.0f)), health);

	renderImage(scaleUI(vec2(355, -48)), scaleUI(vec2(128, 128)), 1);
	drawInt(scaleUI(vec2(400, 24)), int(scaleUI(25.0f)), energy);


	//Other UI stuff
	drawCrosshair();

	drawHeldItem();



	vec4 finalFragColour = vec4(fragColour.rgb, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}