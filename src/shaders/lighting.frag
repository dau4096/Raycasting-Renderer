/* shadows.frag */
#version 460 core


//CameraData
uniform float maxRayDistance;
uniform vec2 textureScale;
uniform vec3 textureOffset;
uniform bool useMipMapping;

//PlayerData
uniform vec3 playerPosition;

//Headlamp
uniform bool headLampEnabled;
uniform int headLampFlicker;

//Sun
uniform vec3 sunDirection;
uniform vec3 sunColour;

//Debug
uniform int debugMode;

//Other
uniform int numVisplanes;
uniform int numWalls;
uniform int numDisplacements;
uniform int numLights;
uniform ivec2 shadowResolution;
uniform bool allowTransparency;


layout(binding=0) uniform sampler2D positionMap;
layout(binding=1) uniform sampler2D normalMap;
layout(binding=2) uniform sampler2DArray textureArray;
layout(rgba32f, binding=0) uniform image2D lightMap;

struct Visplane {
	vec2 start;			//2D start point
	vec2 end;			//2D end point
	float height;		//1D height (Z)
	int textureID;		//Texture ID
	uint textureData;	//Texture formatting data.
	float _padding;		//Buffer Padding
};
layout(std430, binding=0) buffer visplaneSSBO {
	Visplane visplanes[];
};

struct Wall {
	vec3 start;			//3D start point
	vec3 end;			//3D end point
	vec2 direction;		//2D Direction
	int textureID;		//Texture ID
	uint textureData;	//Texture formatting data
};
layout(std430, binding=1) buffer wallSSBO {
	Wall walls[];
};

struct Displacement {
	vec4 vertices[3];
	vec2 UV[3];
	vec4 normal_texID;
};
layout(std430, binding=5) buffer displacementSSBO {
	Displacement displacements[];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	bool enabled;		//Light Validity.
	float _padding;		//Light Padding.
};
layout(std430, binding=3) buffer lightSSBO {
	Light lights[];
};




vec2 fragPosition;

const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.175f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);





void unpackTextureFormattingBits(
		uint inputBits, out bvec2 isWorldspace,
		out vec2 textureScale, out vec2 textureOffset
	) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	isWorldspace.x; (bool) [0 / 1]
		1000 0000 0000 0000 0000 0000 0000 0000
	- isWorldspace.y; (bool) [0 / 1]
		0100 0000 0000 0000 0000 0000 0000 0000
	- textureScale.x; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0011 1111 1100 0000 0000 0000 0000 0000
	- textureScale.y; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0000 0000 0011 1111 1100 0000 0000 0000
	- textureOffset.x; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0011 1111 1000 0000
	- textureOffset.y; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0000 0000 0111 1111
	*/

	isWorldspace = bvec2(
		bool(inputBits & 0x80000000),
		bool(inputBits & 0x40000000)
	);

	textureScale = vec2(
		float((inputBits >> 22) & 0xFF),
		float((inputBits >> 14) & 0xFF)
	) / 16.0f;

	textureOffset = vec2(
		float((inputBits >> 7) & 0x7F),
		float((inputBits >> 0) & 0x7F)
	) / 128.0f;
}




//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}




//Walls
bool quickIntersect(vec3 pointA, vec3 pointB, Wall wall) {
	vec2 rayDelta = pointB.xy - pointA.xy;
	vec2 wallDelta = wall.end.xy - wall.start.xy;

	float denom = cross2D(rayDelta, wallDelta);
	if (abs(denom) < 1e-4f) {return false;}

	vec2 rel = wall.start.xy - pointA.xy;
	float t = cross2D(rel, wallDelta) / denom;
	float u = cross2D(rel, rayDelta) / denom;

	if ((t < -1e-4f) || (t > 1.0f + 1e-4f) || (u < -1e-4f) || (u > 1.0f + 1e-4f)) {return false;}

	float z = pointA.z + (pointB.z - pointA.z) * t;

	return (min(wall.start.z, wall.end.z) - 1e-4f <= z) && (z <= max(wall.start.z, wall.end.z) + 1e-4f);
}

vec2 getWallUV(Wall thisWall, dvec3 intersectPoint3D) {
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;


	bvec2 useWorldSpace;
	vec2 textureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits(
		thisWall.textureData, useWorldSpace,
		textureScale, textureOffset
	);



	//xUV calculation.
	double xUV;
	if (useWorldSpace.x) {
		if (abs(thisWall.direction.y) > abs(thisWall.direction.x)) {
			xUV = fract(intersectPoint3D.y / textureScale.x);
		} else {
			xUV = fract(intersectPoint3D.x / textureScale.x);
		}
		if (xUV < 0.0f) {xUV = 1.0 - abs(xUV);}
	} else {
		if (abs(thisWall.direction.y) > abs(thisWall.direction.x)) {
			xUV = (intersectPoint3D.y - thisWall.start.y) / (thisWall.end.y - thisWall.start.y);
		} else {
			xUV = (intersectPoint3D.x - thisWall.start.x) / (thisWall.end.x - thisWall.start.x);
		}
		xUV = fract(xUV / textureScale.x);
	}


	//yUV calculation.
	double yUV;
	if (useWorldSpace.y) {
		yUV = fract(intersectPoint3D.z / textureScale.y);
		if (yUV < 0.0f) {yUV = 1.0f - abs(yUV);}
	} else {

	}
	

	return vec2(xUV, yUV) + textureOffset.xy;
}



//Visplanes
vec2 getVisplaneUV(vec3 position3D, Visplane plane) {
	bvec2 useWorldSpace;
	vec2 textureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits(
		plane.textureData, useWorldSpace,
		textureScale, textureOffset
	);

	float xUV;
	if (useWorldSpace.x) {
		xUV = fract(position3D.x / textureScale.x);
		if (xUV < 0.0f) {xUV = 1.0f - abs(xUV);}
	} else {
		xUV = (position3D.x - plane.start.x) / (plane.end.x - plane.start.x);
		xUV = fract(xUV / textureScale.x);
	}

	float yUV;
	if (useWorldSpace.y) {
		yUV = fract(position3D.y / textureScale.y);
		if (yUV < 0.0f) {yUV = 1.0f - abs(yUV);}
	} else {
		yUV = (position3D.y - plane.start.y) / (plane.end.y - plane.start.y);
		yUV = fract(yUV / textureScale.y);
	}

	return vec2(xUV, yUV) + textureOffset.xy;
}



//Displacements
float edge(vec2 a, vec2 b, vec2 c) {
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

vec3 barycentricWeights(vec2 v1, vec2 v2, vec2 v3) {
	float areaABC = edge(v1, v2, v3);
	float a = edge(fragPosition, v2, v3)/areaABC;
	float b = edge(fragPosition, v3, v1)/areaABC;
	float c = 1.0f - a - b;
	return vec3(a,b,c);
}





//LOS
bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointB - pointA;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		if (idx == thisIndex && foundType == 1) {continue; /* Wall is the index calling the LOS check. */}
		vec2 wallNormal = vec2(-thisWall.direction.y, thisWall.direction.x);
		float projStart = dot(pointA.xy-thisWall.start.xy, wallNormal);
		float projEnd = dot(pointB.xy-thisWall.start.xy, wallNormal);
		if (projStart * projEnd >= 0.0f) {continue; /* Ray never crosses wall. */}

		double t = (projStart) / (projEnd - projStart);
		dvec3 intersectPoint = pointA - LOSDelta * t;
		vec3 minWall = min(thisWall.start, thisWall.end);
		vec3 maxWall = max(thisWall.start, thisWall.end);
		if (
		    intersectPoint.x + EPSILON_ALT < minWall.x || intersectPoint.x - EPSILON_ALT > maxWall.x ||
		    intersectPoint.y + EPSILON_ALT < minWall.y || intersectPoint.y - EPSILON_ALT > maxWall.y ||
		    intersectPoint.z + EPSILON_ALT < minWall.z || intersectPoint.z - EPSILON_ALT > maxWall.z
		) {
			//Outside of valid wall segment.
			continue;
		}

		if (allowTransparency) {
			vec2 UV = getWallUV(thisWall, vec3(intersectPoint));
			if (textureLod(textureArray, vec3(UV.xy, thisWall.textureID), 0.0f).a >= 0.5f) {
				return true;
			}
		} else {
			return true;
		}
	}


	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<numVisplanes; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (idx == thisIndex && foundType == 2) {continue; /* Visplane is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		if (abs(LOSDelta.z) < EPSILON) {continue;}

		double tFrac = (thisPlane.height - pointA.z) / LOSDelta.z;
		if (tFrac <= 0.0 || tFrac >= 1.0) {continue;}

		dvec3 intersectPoint = pointA + LOSDelta * tFrac;
		if (
			intersectPoint.x >= min(thisPlane.start.x, thisPlane.end.x) - EPSILON &&
			intersectPoint.x <= max(thisPlane.start.x, thisPlane.end.x) + EPSILON &&
			intersectPoint.y >= min(thisPlane.start.y, thisPlane.end.y) - EPSILON &&
			intersectPoint.y <= max(thisPlane.start.y, thisPlane.end.y) + EPSILON
		) {
			if (allowTransparency) {
				vec2 UV = getVisplaneUV(vec3(intersectPoint), thisPlane);
				if (textureLod(textureArray, vec3(UV.xy, thisPlane.textureID), 0.0f).a >= 0.5f) {
					return true;
				}
			} else {
				return true;
			}
		}
	}

	for (int idx=0; idx<numDisplacements; idx++) {
		Displacement thisDisp = displacements[idx];
		if (idx == thisIndex && foundType == 3) {continue; /* Displacement is the index calling the LOS check. */}

		vec3 edgeA = thisDisp.vertices[1].xyz - thisDisp.vertices[0].xyz;
		vec3 edgeB = thisDisp.vertices[2].xyz - thisDisp.vertices[0].xyz;
		vec3 rayCrossEdgeB = cross(LOSDelta, edgeB);
		float det = dot(edgeA, rayCrossEdgeB);

		if (det > -EPSILON && det < EPSILON) {continue; /* Near parallel */}

		float invDeterminant = 1.0f / det;
		vec3 s = pointA - thisDisp.vertices[0].xyz;
		float u = invDeterminant * dot(s, rayCrossEdgeB);
	    if ((u < 0 && abs(u) > EPSILON) || (u > 1 && abs(u-1) > EPSILON)) {
	        continue; //No hit.
	    }

	    vec3 sCrossEdgeA = cross(s, edgeA);
	    float v = invDeterminant * dot(LOSDelta, sCrossEdgeA);
	    if ((v < 0 && abs(v) > EPSILON) || (u + v > 1 && abs(u + v - 1) > EPSILON)) {
	    	continue; //No hit.
	    }

	    float t = invDeterminant * dot(edgeB, sCrossEdgeA);
	    if (t > EPSILON) {
	    	return true;
	    }
 	}

	return false;
}




float getNormalDot(vec3 normal, vec3 direction) {
	if (dot(normal, normal) < EPSILON) {return 1.0f; /* When normal is invalid (all 0s, used on sprites/sky where no normal exists) then treat it as 1.0f. */}
	return dot(normal, direction);
}


void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	vec2 thisFramePosition = vec2(fragPosition / vec2(shadowResolution));
	vec4 data = texture(positionMap, thisFramePosition);
	vec3 origin = data.xyz;

	int iData = int(data.w);
	int type = iData & 0x3;
	int index = iData >> 2;

	vec3 normal = texture(normalMap, thisFramePosition).xyz;
	

	if ((debugMode > 0) || (type == 0)) {
		imageStore(lightMap, framePosition, vec4(1.0f, 1.0f, 1.0f, 1.0f));
		return;
	}



	vec3 lightColour = vec3(0.0f, 0.0f, 0.0f);

	//Light effect
	for (int idx=0; idx<numLights; idx++) {
		//Iterate through all lights.
		Light thisLight = lights[idx];
		if (!thisLight.enabled) {continue;}
		vec3 delta = origin - thisLight.position;
		float distSQ = dot(delta, delta);
		float attenuation = max(0.0, 1.0 - abs(distSQ / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance to light.
		if (attenuation < 0.0f) {continue;}
		vec3 lightDir = normalize(thisLight.position - origin);
		float normalDot = getNormalDot(normal, lightDir);
		if (normalDot <= EPSILON) {continue;}

		
		//Shadow Checks
		bool inShadow = checkLOS(thisLight.position, origin, index, type);

		if (!inShadow) {
			vec3 lightContribution = thisLight.colour * attenuation;
			vec3 litColour = lightContribution;

			lightColour += litColour;
		}
	}


	//Headlamp Effect
	if (headLampEnabled) {
		Light headLamp;
		headLamp.position = playerPosition;
		headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
		headLamp.intensity = 5.0f + (headLampFlicker / 768.0f); //headLampFlicker is 0-255.


		vec3 lightDir = normalize(headLamp.position - origin);
		float normalDot = getNormalDot(normal, lightDir);

		if (normalDot >= 0.0f) {
			float distance = length(origin - headLamp.position);
			float attenuation = max(0.0, 1.0 - ((distance*distance) / (headLamp.intensity*headLamp.intensity))); //Intensity fades with distance to light.
			float brightness = clamp(attenuation, 0.0f, 1.0f);

			vec3 lightContribution = headLamp.colour * brightness;
			vec3 litColour = lightContribution;

			lightColour += litColour;
		}
	}


	//Sun Effect
	float sunNormalDot = getNormalDot(normal, normalize(sunDirection));
	vec3 offset = normal * 0.01f;
	bool sunBlocked = checkLOS(origin + offset, origin + offset + sunDirection * maxRayDistance, index, type);
	if ((sunNormalDot >= 0.0f) && !sunBlocked) {
		float sunNormalContrib = (sunNormalDot * 0.5f) + 0.5f;
		vec3 litColour = sunColour.rgb * sunNormalContrib;
		lightColour += litColour;
	}


	//Minimum/Maximum brightness.
	lightColour = clamp(lightColour, DEFAULT_BRIGHTNESS, 1.75f);


	imageStore(lightMap, framePosition, vec4(lightColour.rgb, 1.0f));
}
