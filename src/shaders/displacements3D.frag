/* displacements.frag */
#version 460 core


in vec3 fragPos3D;
in vec2 fragUV;
in flat int dispIndex;


layout(location=0) out vec4 outColour;
layout(location=1) out vec4 outPosition;
layout(location=2) out vec4 outNormal;


layout(binding=0) uniform sampler2DArray textureArray;

struct Displacement {
	vec4 vertices[3];	//3D Vertices
	vec2 UV[3];			//2D UV coordinates per vertex
	vec4 normal_texID;	//Normal and texture ID packed together
};
layout(std430, binding=4) buffer displacementSSBO {
	Displacement displacements[];
};



uniform vec3 playerPosition;
uniform float shadowMapQuality;
uniform bool allowTransparency;
uniform ivec2 renderResolution;
uniform float verticalFOV;
uniform float rayOffset;
uniform float maxRayDistance;



void main() {
	vec2 fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);

	Displacement thisDisp = displacements[dispIndex];
	vec4 albedo = texture(textureArray, vec3(fragUV, thisDisp.normal_texID.w));
	if (allowTransparency && (albedo.a < 0.5f)) {discard;}

	vec2 d = (playerPosition - fragPos3D).xy;
	float dist = 1.0f / inversesqrt(dot(d,d));
	gl_FragDepth = clamp(dist / maxRayDistance, 0.0f, 1.0f); //2D [XY] distance.


	outColour = vec4(albedo.rgb, dist);
	outPosition = vec4(fragPos3D, dispIndex);
	outNormal = vec4(thisDisp.normal_texID.xyz, 1.0f);
}