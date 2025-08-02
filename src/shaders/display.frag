/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;


layout(binding=0) uniform sampler2D renderedFrameSampler2D;
layout(binding=1) uniform sampler2D interfaceTexture;
layout(binding=2) uniform sampler2DArray lightMapsArray;
layout(binding=3) uniform sampler2D normalMap;
layout(rgba32f, binding=0) writeonly uniform image2D renderedFrameImage2D;


//Camera
uniform float maxRayDistance;
uniform ivec2 screenResolution;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

//Other
uniform int antiAliasingLevel;
uniform bool smoothingEnabled;
uniform int quantisingLevel;
uniform bool screenshotHasHUD;
uniform bool shouldTakeScreenshot;
uniform int numLights;
uniform vec4 screenTint;
uniform bool isInvertEffect;


#define EPSILON 1e-4f

//////////////// Config stuff ////////////////
//Lighting;
#define DEFAULT_BRIGHTNESS 0.175f
#define NUM_PSEUDO_LIGHTS 2
//////////////// Config stuff ////////////////


vec2 getUV(vec2 pos) {
	return vec2(pos) / vec2(screenResolution);
}


vec4 antiAliasFunc() {
	const float edgeThreshold = 1.0f;

    vec2 baseUV = getUV(gl_FragCoord.xy);
    vec4 centrePX = texture(renderedFrameSampler2D, baseUV);
    float centreDepth = centrePX.a;

    float minDepth = centreDepth, maxDepth = centreDepth;

    vec3 colourSum = vec3(0.0f, 0.0f, 0.0f);
    int n = 0;

	for (int dx=-antiAliasingLevel; dx<=antiAliasingLevel; dx++) {
		for (int dy=-antiAliasingLevel; dy<=antiAliasingLevel; dy++) {
            vec2 UV = getUV(gl_FragCoord.xy + vec2(dx, dy));
            vec4 sampledPX = texture(renderedFrameSampler2D, UV);

            float depth = sampledPX.a;
            if (depth < 0.0) {continue; /* fragment was UI */}

            minDepth = min(minDepth, depth);
            maxDepth = max(maxDepth, depth);
            colourSum += sampledPX.rgb;
            n++;
		}
	}

	if ((maxDepth - minDepth) > edgeThreshold) { //Edge found.
		vec3 meanColour = colourSum / float(n);
		return vec4(meanColour.rgb, 1.0f);
	} else { //No edge found.
		return centrePX;
	}

}


vec4 quantisingFunc(vec2 mainUV) {
	float delta = 255.0f / (quantisingLevel - 1.0f);

	//https://en.wikipedia.org/wiki/Quantization_(image_processing)#Grayscale_quantization
	vec4 albedo = texture(renderedFrameSampler2D, mainUV);
	if ((albedo.a == -1.0f) || (albedo.a >= maxRayDistance)) {return albedo; /* UI Element */}
	vec3 qVal = floor(floor((albedo.rgb * 255.0f) / delta) * delta + (delta/2.0f)) / 255.0f;

	return vec4(qVal, 1.0f);
}


vec4 smoothingFunc() {
	int n = 0;
	vec2 UV;
	vec3 colourSum = vec3(0.0f, 0.0f, 0.0f);
	vec4 albedo;

	for (int dx=-1; dx<=1; dx++) {
		for (int dy=-1; dy<=1; dy++) {
			UV = getUV(gl_FragCoord.xy + vec2(dx, dy));
			albedo = texture(renderedFrameSampler2D, UV);
			if (albedo.a != -1) {
				n++;
				colourSum += albedo.rgb;
			}
		}
	}

	UV = getUV(gl_FragCoord.xy);
	vec4 centrePX = texture(renderedFrameSampler2D, UV);
	if (n > 0) {
		return vec4(colourSum / float(n), centrePX.a);
	} else {
		return centrePX;
	}
}


vec3 getBrightness(vec2 UV, out float maxBrightness) {
	if ((debugMode != 0) && (debugMode != 3)) {return vec3(1.0f, 1.0f, 1.0f); /* Not no-debug and not lighting debug. */}

	vec3 lightingSum = vec3(0.0f, 0.0f, 0.0f);
	for (int i=0; i<(numLights+NUM_PSEUDO_LIGHTS); i++) {
		lightingSum += texture(lightMapsArray, vec3(UV.xy, i)).rgb;
	}
	if (length(texture(normalMap, UV).xyz) < EPSILON) {
		//Sprites and sky.
		maxBrightness = 1.0f;
	} else {
		maxBrightness = 2.25f;
	}
	return clamp(lightingSum, DEFAULT_BRIGHTNESS, maxBrightness);
}


void main() {
	vec4 resultant;
	vec2 mainUV = getUV(gl_FragCoord.xy);
	vec4 albedo = texture(renderedFrameSampler2D, mainUV);
	if (albedo.a >= maxRayDistance) {
		if (debugMode == 3) { //Debug lighting.
			resultant = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		} else {
			resultant = vec4(albedo.rgb, 1.0f);
		}
	} else {
		float maxBrightness;
		vec3 brightness = getBrightness(mainUV, maxBrightness);
		if (debugMode == 3) { //Debug lighting.
			resultant.rgb = brightness / maxBrightness;
		} else {
			resultant = vec4(albedo.rgb * brightness, 1.0f);

			if (isInvertEffect) {
				vec3 invert = vec3(1.0f, 1.0f, 1.0f) - resultant.rgb;
				float flashAlpha = screenTint.a * 2.0f - 1.0f;
				resultant.rgb = mix(resultant.rgb, mix(screenTint.rgb, invert.rgb, flashAlpha), screenTint.a);
			} else {
				resultant.rgb = mix(resultant.rgb, screenTint.rgb, screenTint.a);
			}
		}
	}

	if (quantisingLevel > 1) { //Quantising 1 would be 1 colour. Not adequate. Works based on luminance.
		resultant = quantisingFunc(mainUV);
	}

	if (antiAliasingLevel > 0) { //More useful Anti-Aliasing
		resultant = antiAliasFunc();
	} else if (smoothingEnabled) { //Simple Anti-Aliasing
		resultant = smoothingFunc();
	}


	ivec2 framePosition = ivec2((gl_FragCoord.xy * vec2(renderResolution)) / vec2(screenResolution));
	if (!screenshotHasHUD && shouldTakeScreenshot) {
		imageStore(renderedFrameImage2D, framePosition, vec4(resultant.rgb, 1.0f));
	}
	vec4 interfaceColour = texture(interfaceTexture, mainUV);
	fragColour = vec4(mix(resultant.rgb, interfaceColour.rgb, interfaceColour.a), 1.0f);
	if (screenshotHasHUD && shouldTakeScreenshot) {
		imageStore(renderedFrameImage2D, framePosition, fragColour);
	}
}