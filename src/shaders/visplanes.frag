/* visplanes.frag */
#version 460 core

uniform bool zoom;
uniform float playerViewAngle;
uniform vec3 playerPosition;
uniform sampler2DArray textureArray;
uniform int drawUV;


layout(rgba32f, binding=0) uniform image2D renderedFrame;
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

struct Light {
	vec3 position;		//Light Position
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float padding[3];	//Light Padding.
};
layout(std140, binding = 4) uniform lightUBO {
	Light lights[32];
};



vec2 fragPosition;
ivec2 renderResolution;
vec4 fragColour;
float t;
const vec3 INVALID = vec3(1e30f, 1e30f, 1e30f);

float angleClamp(float value) {
	if (value < 0.0f) {
		return 360.0f + value;
	}
	return mod(value, 360.0f);
}



vec3 getUVCoords(float floorZ, float ceilingZ) {
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;
	bool topHalf = fragPosition.y > renderResolution.y/2;
	float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));

	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0; // Normalized screen Y [-1, 1]
	float vAO = normY * (verticalFOV/2);
	float theta = radians(playerViewAngle + rayOffset);

	
	const float EPSILON = 1e-6f;
	const vec3 planeNormal = vec3(0.0f, 0.0f, 1.0f);
	vec3 rayDirection = normalize(vec3(
		sin(theta), cos(theta),
		tan(radians(vAO))
	));

	float denom = dot(planeNormal, rayDirection);
	if (abs(denom) < EPSILON) {return INVALID; /* Nearly parallel. */}

	vec3 planeOrigin = (topHalf) ? vec3(0.0f, 0.0f, ceilingZ) : vec3(0.0f, 0.0f, floorZ);
	vec3 planeDir = planeOrigin - playerPosition;

	t = dot(planeDir, planeNormal) / denom;
	if (t <= 0.0f) {return INVALID; /* Behind Ray origin. */}

	vec3 intersectPoint = playerPosition + ((t/maxRayDistance) * rayDirection);
	vec2 realPosition = intersectPoint.xy; //Position in 2D space, as Z is already known.
	//Compare against some XY bounds maybe.


	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = fract(realPosition.x);
	float yUV = fract(realPosition.y);
	if (!topHalf) { xUV = 1.0 - xUV; }  // Only flip when needed
	//Texture index depends on top (ceiling) or bottom (floor) half.
	float index = (topHalf) ? topIndex : lowIndex;

	return vec3(xUV, yUV, index);
}


void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);


	const float floorZ = -1.0f;
	const float ceilingZ = 1.0f;


	vec3 UVcoords = getUVCoords(floorZ, ceilingZ);

	if (UVcoords == INVALID) {
		fragColour = vec4(1.0f, 0.0f, 1.0f, 1.0f);
	} else if (false) {
		fragColour = vec4(UVcoords.xy, UVcoords.z/2, 1.0); // Visualize UV coords
	} else {
		float distanceFade = 1.0f - (t / (maxRayDistance * maxRayDistance));
		fragColour = texture(textureArray, UVcoords) * distanceFade; //Draw texture colour.
	}

	//Write the colour to the frame.
	ivec2 framePosition = ivec2(fragPosition);
	vec4 finalFragColour = vec4(fragColour.rgb, maxRayDistance);
	depths[framePosition.x] = maxRayDistance;
	imageStore(renderedFrame, framePosition, finalFragColour);
}
