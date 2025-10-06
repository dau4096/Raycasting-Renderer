/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;


layout(binding=0) uniform sampler2D renderedFrameSampler2D;
layout(binding=1) uniform sampler2D depthMap;
layout(binding=2) uniform sampler2D interfaceTexture;
layout(binding=3) uniform sampler2DArray lightMapsArray;
layout(binding=4) uniform sampler2D positionMap;
layout(binding=5) uniform sampler2D normalMap;

layout(rgba32f, binding=0) writeonly uniform image2D frameToScreenshot;


//Camera
uniform float maxRayDistance;
uniform ivec2 screenResolution;
uniform ivec2 renderResolution;
uniform vec3 playerPosition;

//Sky
uniform vec3 fogColour;

//Debug
uniform int debugMode;

//Other
uniform bool antiAliasing;
uniform int quantisingLevel;
uniform bool useLighting;
uniform bool screenshotHasHUD;
uniform bool shouldTakeScreenshot;
uniform int numLights;
uniform vec4 screenTint;
uniform bool isInvertEffect;


#define EPSILON 1e-4f

//////////////// Config stuff ////////////////
//Lighting;
#define MIN_BRIGHTNESS 0.175f
#define MAX_BRIGHTNESS 2.25f
#define NUM_PSEUDO_LIGHTS 2
//////////////// Config stuff ////////////////


vec2 getUV(vec2 pos) {
	return vec2(pos) / vec2(screenResolution);
}


const vec2 offsets[8] = {
	vec2(-1.0f, -1.0f), vec2( 0.0f, -1.0f), vec2( 1.0f, -1.0f),
	vec2(-1.0f,  0.0f),                     vec2( 1.0f,  0.0f),
	vec2(-1.0f,  1.0f), vec2( 0.0f,  1.0f), vec2( 1.0f,  1.0f)
};
vec4 antiAliasFunc(vec2 mainUV, vec3 centreColour, vec4 positionData) {
	//Screenspace custom AA based on whether a pixel is an edge between 2 different surfaces or not.
	vec2 inverseScreenRes = 1.0f / vec2(screenResolution);
	
	int centreIData = int(positionData.w); //Contains object index and type, encoded as bits.
	vec3 colourSum = centreColour.rgb;
	bool isEdge = false;

	for (uint offsetIndex=0; offsetIndex<8; offsetIndex++) {
		vec2 thisUV = mainUV + (offsets[offsetIndex] * inverseScreenRes);
		int thisIData = int(texture(positionMap, thisUV).w);
		colourSum += texture(renderedFrameSampler2D, thisUV).rgb;
		if (abs(thisIData - centreIData) > 0) {
			//Different object/surface was hit. Apply anti-aliasing.
			isEdge = true;
		}
	}

	//Rather unfortunate how large this is.
	return vec4(
		mix(
			mix(
				centreColour.rgb,
				vec3(0.0f, 0.0f, 0.0f),
				int(debugMode == 5)
			),
			mix(
				colourSum / 8.0f,
				vec3(1.0f, 0.0f, 1.0f),
				int((debugMode == 4) || (debugMode == 5))
			),
			int(isEdge)
		),
		1.0f
	);
}



bool accessBit(uint value, uint n) {
	return bool((value >> n) & 0x1u);
}

uint getMaterial(uint typeAndMaterialID) {
	/*
	-- Possible Materials --
	MAT_FLAT		|	Default material, does not expect normal map.
	MAT_NORMAL		|	Default material. Acts as all surfaces do now.
	MAT_DIFFRACT	|	Diffracts whatever is behind it.
	MAT_LIQUID		|	Wiggly and wavey.
	MAT_SHINY		|	Has specular reflection.
	MAT_MATT		|	Does not have specular light reflection.
	MAT_SKY			|	Like W_PORTAL/V_PORTAL currently render.
	MAT_INVERSE		|	Applies the display shader's inverse effect to whatever is behind it.
	MAT_FABRIC		|	Like MAT_LIQUID but more flag-like?
	MAT_HALL		|	"Hall of mirrors", do not clear previous frame's pixel colour?
	MAT_FULLBRIGHT	|	No lighting.
	MAT_SHELL		|	Fake shell texturing for grass or whatever.

	Could make these specific bit flags;
	0bABCDEFFF
	A: has normal
	B: diffract
	C: shadow
	D: lighting
	E: use specular
	FFF: index for SKY/FABRIC/HALL/INVERSE/SHELL etc.
	i.e. current normal map mat would be 	:   0b10011000 	:  152 	: 0x98
	current portal sky mat would be			:   0b00000001 	:   33 	: 0x21
	some sort of liquid 					:   0b01111000	:  120	: 0x78
	current sprites							:	0b10010000	:  144	: 0x90
	*/
	return typeAndMaterialID & 0xFFu;
}

#define DIFFRACTION_SCALING vec2(200.0f, 0.1f)
vec4 getAlbedo(vec2 mainUV, vec4 positionData) {
	vec4 normalMapData = texture(normalMap, mainUV);
	uint material = getMaterial(uint(normalMapData.a));

	if (accessBit(material, 5u)) { //Diffraction
		vec3 surfaceDirection = vec3(-normalMapData.y, normalMapData.x, 0.0f);
		vec3 delta = positionData.xyz - playerPosition;
		vec3 dirToFrag = normalize(delta);

		float horizontalDot = dot(surfaceDirection.xy, dirToFrag.xy);
		float xOffset = DIFFRACTION_SCALING.x * horizontalDot / renderResolution.x;

		float yOffset = DIFFRACTION_SCALING.y * delta.y / renderResolution.y;

		vec2 UVoffset = vec2(xOffset, yOffset);
		return vec4(UVoffset, normalMapData.a, 0.0f);
		vec2 newUV = mainUV + UVoffset;
		float newSurfaceData = texture(positionMap, newUV).a;
		if (newSurfaceData == positionData.a) {
			//return texture(renderedFrameSampler2D, newUV).rgba;
		}

	}
	return texture(renderedFrameSampler2D, mainUV).rgba;
}


vec4 quantisingFunc(vec2 mainUV) {
	float delta = 255.0f / (quantisingLevel - 1.0f);

	//https://en.wikipedia.org/wiki/Quantization_(image_processing)#Grayscale_quantization
	vec4 albedo = texture(renderedFrameSampler2D, mainUV);
	if ((albedo.a == -1.0f) || (albedo.a >= maxRayDistance)) {return albedo; /* UI Element */}
	vec3 qVal = floor(floor((albedo.rgb * 255.0f) / delta) * delta + (delta/2.0f)) / 255.0f;

	return vec4(qVal, 1.0f);
}



vec3 getBrightness(vec2 UV) {
	if (!useLighting || ((debugMode != 0) && (debugMode != 3))) {return vec3(1.0f, 1.0f, 1.0f); /* Not no-debug and not lighting debug. */}

	vec3 lightingSum = vec3(0.0f, 0.0f, 0.0f);
	for (int i=0; i<(numLights+NUM_PSEUDO_LIGHTS); i++) {
		lightingSum += texture(lightMapsArray, vec3(UV.xy, i)).rgb;
	}
	return clamp(lightingSum, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
}


void main() {
	vec4 resultant;
	vec2 mainUV = getUV(gl_FragCoord.xy);


	vec4 positionData = texture(positionMap, mainUV);
	vec4 albedo = getAlbedo(mainUV, positionData);


	float fragDistance = texture(depthMap, mainUV).r * maxRayDistance;
	if (fragDistance >= maxRayDistance) {
		if (debugMode == 3) { //Debug lighting.
			resultant = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		} else {
			resultant = vec4(albedo.rgb, 1.0f);
		}
		if (isInvertEffect) {
			vec3 invert = vec3(1.0f, 1.0f, 1.0f) - resultant.rgb;
			float flashAlpha = screenTint.a * 2.0f - 1.0f;
			resultant.rgb = mix(resultant.rgb, mix(screenTint.rgb, invert.rgb, flashAlpha), screenTint.a);
		} else {
			resultant.rgb = mix(resultant.rgb, screenTint.rgb, screenTint.a);
		}
	} else {
		vec3 brightness = getBrightness(mainUV);
		if (debugMode == 3) { //Debug lighting.
			resultant.rgb = brightness / MAX_BRIGHTNESS;
		} else {
			vec3 resultantTMP = vec3(albedo.rgb * brightness);

			if (isInvertEffect) {
				vec3 invert = vec3(1.0f, 1.0f, 1.0f) - resultantTMP.rgb;
				float flashAlpha = screenTint.a * 2.0f - 1.0f;
				resultantTMP.rgb = mix(resultantTMP.rgb, mix(screenTint.rgb, invert.rgb, flashAlpha), screenTint.a);
			} else {
				resultantTMP.rgb = mix(resultantTMP.rgb, screenTint.rgb, screenTint.a);
			}

			float fogAlpha = (fragDistance / maxRayDistance);
			fogAlpha *= fogAlpha;
			resultant = vec4(mix(resultantTMP.rgb, fogColour, fogAlpha), 1.0f);
		}
	}

	if (quantisingLevel > 1) { //Quantising 1 would be 1 colour. Not allowed. Works based on luminance.
		resultant = quantisingFunc(mainUV);
	}

	if (antiAliasing) { //More useful Anti-Aliasing
		resultant = antiAliasFunc(mainUV, resultant.rgb, positionData);
	}


	ivec2 framePosition = ivec2((gl_FragCoord.xy * vec2(renderResolution)) / vec2(screenResolution));
	vec4 interfaceColour = texture(interfaceTexture, mainUV);

	//TextObjects use negative distance as their alpha. General UI uses 0-1 alpha values.
	bool isTextObject = interfaceColour.a == 2.0f;
	interfaceColour.a = (isTextObject) ? 1.0f : interfaceColour.a;

	//Final fragment output (blend of scene and UI)
	fragColour = vec4(mix(resultant.rgb, interfaceColour.rgb, interfaceColour.a), 1.0f);

	if (shouldTakeScreenshot) {
		//Check if the pixel references a TextObject or HUD before choosing between the interface colour or environment colour.
		vec4 screenshotColour = vec4(
			mix(
				mix(
					resultant.rgb,
					interfaceColour.rgb,
					int(isTextObject)
				),
				fragColour.rgb,
				int(screenshotHasHUD)
			),
			1.0f
		);
		imageStore(frameToScreenshot, framePosition, screenshotColour);
	}

	/*
	//Motion extraction test.
	vec4 oldCol = imageLoad(frameToScreenshot, framePosition);
	imageStore(frameToScreenshot, framePosition, fragColour);

	vec4 inv = 1.0f - oldCol;
	fragColour = (fragColour * 0.5f) + (inv * 0.5f);
	*/
}