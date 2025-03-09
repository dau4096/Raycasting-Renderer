/* interface.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec3 playerPosition;
uniform bool zoom;

vec2 fragPosition;
ivec2 renderResolution;
vec3 fragColour;
float fragDepth;
float fragDepths[1920];


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
    float zoomFactor;
    float maxRayAngle;
    float maxRayDistance;

    float topIndex;
    float lowIndex;

    vec2 textureSize;

    float padding[2];
};
layout(std430, binding = 5) buffer depthBuffer {
	float depths[];
};


bool isClockwise(vec2 point, vec2 vector) {
	return (point.y*vector.x) - (point.x*vector.y) > 0.0f;
}


vec3 viewMap(float rayAngle) {
	vec2 centre = vec2(renderResolution.x * 0.875f, renderResolution.y * 0.5f);
	const float radius = renderResolution.y / 32.5f;
	float radiusSquared = radius * radius;

	vec2 sectorStart = vec2(sin(radians(rayAngle)), cos(radians(rayAngle)));
	vec2 sectorEnd = vec2(-sectorStart.x, sectorStart.y);
	vec2 fragRelativePosition = fragPosition - centre;

	bool isInStartRange = !isClockwise(sectorStart, fragRelativePosition);
	bool isInEndRange = isClockwise(sectorEnd, fragRelativePosition);
	float fragUIDistance = length(fragRelativePosition); //Distance from centre to fragment.



	if (isInStartRange && isInEndRange && fragUIDistance <= radiusSquared) {
		float dotProd = 1.0f - dot(normalize(sectorEnd), normalize(fragRelativePosition));
		float range = 1.0f - dot(normalize(sectorEnd), normalize(sectorStart));
		float angle = dotProd / range;
	    int index = int(angle * float(renderResolution.x));
	    index = clamp(index, 0, renderResolution.x - 1);
	    float thisFragDistance = depths[index] / (maxRayDistance / 4.0f);
	    float fragUIDistanceScaled = fragUIDistance/radiusSquared;
	    
		vec3 partialColour = (fragUIDistanceScaled < thisFragDistance) ? vec3(0.75f, 0.75f, 0.75f) : vec3(0.75f-(0.5*(fragUIDistanceScaled-thisFragDistance)), 0.25f, 0.25f);
		return (abs(thisFragDistance - fragUIDistanceScaled) <= 0.05) ? vec3(0.0f, 0.0f, 0.0f) : partialColour; //If close enough to a wall, show as black.
	}
	return vec3(1e3f, 1e3f, 1e3f);
}




void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);
	vec4 imageColour = imageLoad(renderedFrame, framePosition);
	fragColour = imageColour.rgb;
	fragDepth = imageColour.a;

	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	vec3 mapColour = viewMap(rayAngle);
	if (mapColour != vec3(1e3f, 1e3f, 1e3f)) {
		fragColour = mapColour;
	}







	vec4 finalFragColour = vec4(fragColour.rgb, fragDepth);
	imageStore(renderedFrame, framePosition, finalFragColour);
}