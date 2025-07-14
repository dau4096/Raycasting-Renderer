/* projection.vert */
#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
layout(location=3) in float aIdx;


uniform vec3 playerPosition;
uniform float playerViewAngle;
uniform bool zoom;
uniform float zoomFactor;
uniform float maxRayAngle;
uniform float maxRayDistance;
uniform float verticalFOV;
uniform ivec2 renderResolution;
uniform float playerViewRoll;
uniform float playerViewPitch;


out vec3 fragPos3D;
out vec2 fragUV;
out flat int dispIndex;


void main() {
	vec2 delta2D = (aPos - playerPosition).xy;
	float zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float halfFOV = maxRayAngle / zoomEffect;
	float inverseDist = inversesqrt(dot(delta2D, delta2D));

	//X Coordinate;
	vec2 direction2D = normalize(delta2D);
	float theta = atan(direction2D.x, direction2D.y);
	float angleDelta = degrees(theta) - playerViewAngle;
	if (angleDelta > 180.0f) {angleDelta -= 360.0f;}
	if (angleDelta < -180.0f) {angleDelta += 360.0f;}
	float x = (angleDelta / (halfFOV));


	//Y Coordinate;
	float invDistance = 1.5f * zoomEffect * inverseDist;
	float y = ((aPos.z - playerPosition.z) * 0.8f / tan(verticalFOV / 2.0f)) * invDistance;

	//Screen warping
	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
	y += x * rollDecimal * 2.0f;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f) * zoomEffect;
	y += pitchDecimal / 27.0f; //Scaling to resolution. 10px per degree if it's 540px tall.


	float z = 1.0f / (inverseDist * maxRayDistance);

	gl_Position = vec4(x, y, z, 1.0f);

	fragPos3D = aPos;
	fragUV = aUV;
	dispIndex = int(aIdx);
}