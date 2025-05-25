/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;

layout(rgba32f, binding = 0) uniform image2D renderedFrameWriteOnly;

uniform float maxRayDistance;
uniform ivec2 screenResolution;
uniform sampler2D renderedFrame;
uniform int antiAliasingLevel;
uniform bool smoothingEnabled;
uniform int quantisingLevel;
uniform bool screenshotHasHUD;


const float EPSILON = 1e-4f;


vec4 antiAliasFunc() {
	const float edgeThreshold = 1.0f;

    vec2 baseUV = gl_FragCoord.xy / vec2(screenResolution);
    vec4 centrePX = texture(renderedFrame, baseUV);
    float centreDepth = centrePX.a;

    float minDepth = centreDepth, maxDepth = centreDepth;

    vec3 colourSum = vec3(0.0f, 0.0f, 0.0f);
    int n = 0;

	for (int dx=-antiAliasingLevel; dx<=antiAliasingLevel; dx++) {
		for (int dy=-antiAliasingLevel; dy<=antiAliasingLevel; dy++) {
            vec2 uv = (gl_FragCoord.xy + vec2(dx, dy)) / vec2(screenResolution);
            vec4 sampledPX = texture(renderedFrame, uv);

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
	vec4 albedo = texture(renderedFrame, mainUV);
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
			UV = (gl_FragCoord.xy + vec2(dx, dy)) / vec2(screenResolution);
			albedo = texture(renderedFrame, UV);
			if (albedo.a != -1) {
				n++;
				colourSum += albedo.rgb;
			}
		}
	}

	UV = gl_FragCoord.xy / vec2(screenResolution);
	vec4 centrePX = texture(renderedFrame, UV);
	if (n > 0) {
		return vec4(colourSum / float(n), centrePX.a);
	} else {
		return centrePX;
	}
}


void main() {
	vec4 resultant;
	vec2 mainUV = gl_FragCoord.xy / vec2(screenResolution);
	resultant = vec4(texture(renderedFrame, mainUV).rgb, 1.0f);

	if (quantisingLevel > 1) { //Quantising 1 would be 1 colour. Not adequate. Works based on luminance.
		resultant = quantisingFunc(mainUV);
	}

	if (antiAliasingLevel > 0) { //More useful Anti-Aliasing
		resultant = antiAliasFunc();
	} else if (smoothingEnabled) { //Simple Anti-Aliasing
		resultant = smoothingFunc();
	}

	fragColour = vec4(resultant.rgb, 1.0f);
	if (screenshotHasHUD) {
		imageStore(renderedFrameWriteOnly, ivec2(gl_FragCoord.xy), vec4(resultant.rgb, 1.0f));
	}
}