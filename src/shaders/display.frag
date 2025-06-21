/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;


layout(binding=0) uniform sampler2D renderedFrameSampler2D;
layout(binding=1) uniform sampler2D interfaceTexture;
layout(binding=2) uniform sampler2D lightMap;
layout(rgba32f, binding=0) uniform image2D renderedFrameImage2D;


//Camera
uniform float maxRayDistance;
uniform ivec2 screenResolution;
uniform ivec2 renderResolution;

//Other
uniform int antiAliasingLevel;
uniform bool smoothingEnabled;
uniform int quantisingLevel;
uniform bool screenshotHasHUD;


const float EPSILON = 1e-4f;

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


void main() {
	vec4 resultant;
	vec2 mainUV = getUV(gl_FragCoord.xy);
	vec4 albedo = texture(renderedFrameSampler2D, mainUV);
	if (albedo.a >= maxRayDistance) {
		resultant = vec4(albedo.rgb, 1.0f);
	} else {
		vec3 brightness = texture(lightMap, mainUV).rgb;
		resultant = vec4(albedo.rgb * brightness, 1.0f);
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
	if (!screenshotHasHUD) {
		imageStore(renderedFrameImage2D, framePosition, vec4(resultant.rgb, 1.0f));
	}
	vec4 interfaceColour = texture(interfaceTexture, mainUV);
	fragColour = vec4(mix(resultant.rgb, interfaceColour.rgb, interfaceColour.a), 1.0f);
	if (screenshotHasHUD) {
		imageStore(renderedFrameImage2D, framePosition, fragColour);
	}
}