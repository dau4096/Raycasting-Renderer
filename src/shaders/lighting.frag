/* shadows.frag */
#version 460 core


//CameraData
uniform float maxRayDistance;

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


layout(binding=0) uniform sampler2D positionMap;
layout(binding=1) uniform sampler2D normalMap;
layout(rgba32f, binding=0) uniform image2D lightMap;

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	vec2 _padding;		//Visplane Padding
};
layout(std430, binding=0) buffer visplaneSSBO {
	Visplane visplanes[];
};

struct Wall {
	vec3 start;		//Wall Start.
	vec3 end;		//Wall End.
	vec2 direction;	//Wall 2D Direction
	int textureID;	//Wall Texture.
	float _padding;	//Wall Validity.
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



//LOS
bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointB - pointA;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		if (idx == thisIndex && foundType == 1) {continue; /* Wall is the index calling the LOS check. */}

		bool blocked = quickIntersect(pointA, pointB, thisWall);
		if (blocked) {return true;}
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
			return true;
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
