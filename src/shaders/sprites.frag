/* sprites.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;

//Player Data
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform bool zoom;
uniform ivec2 renderResolution;

//Debug
uniform int drawUV;

//Headlamp
uniform bool headLampEnabled;
uniform int headLampFlicker;

//Sun
uniform vec3 sunDirection;
uniform vec3 sunColour;

//Other
uniform int numVisplanes;
uniform int numWalls;
uniform int numDisplacements;
uniform int numSprites;
uniform int numLights;


layout(rgba32f, binding=0) uniform image2D renderedFrame;

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

struct Sprite {
	vec3 position;	//Sprite Position.
	float width;	//Sprite Width.
	float height;	//Sprite Height.
	int textureID;	//Sprite Texture ID.
	int centreX;	//Sprite Screen Centre.
};
layout(std430, binding=2) buffer spriteSSBO {
	Sprite sprites[];
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




struct Ray {
	vec2 position, direction, end;
};

Ray createRay(vec2 position, vec2 direction, float maxDist=maxRayDistance) {
	Ray ray;
	ray.position = position;
	ray.direction = direction;
	ray.end = ray.position + (ray.direction * maxDist);
	return ray;
};


vec2 fragPosition;
vec4 fragColour;
float fragDepth, tanVerticalViewAngleOffset, zoomEffect;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.25f;
const float HEADLAMP_MIN_LIGHT = 0.1f;
const vec2 INVALIDv2 = vec2(1e30f, 1e30f);
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec3 INVALIDv3 = vec3(1e30f, 1e30f, 1e30f);

const bool noLighting = false;

const float mipMapLevels = 7.0f;
const float minMipMapDistance = 5.0f;



//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}

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



vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth) {
	float spriteFootZ = thisSprite.position.z - thisSprite.height/2.0f;
	float spriteHeadZ = thisSprite.position.z + thisSprite.height/2.0f;

	float zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float distance = length(playerPosition.xy - thisSprite.position.xy) / zoomEffect;
	float projectedYLow = (playerPosition.z - spriteFootZ) / distance;
	float projectedYTop = (playerPosition.z - spriteHeadZ) / distance;
	
	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	float screenYTop = renderResolution.y * (0.5 - projectedYTop);

	float spriteHeight = screenYTop - screenYLow;
	float spriteWidth = thisSprite.width * spriteHeight;
	spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;



	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteWidth/2);
	float xUV = fract(relativeX / spriteWidth);
	if (fragPosition.x < centrePixelX - (spriteWidth/2) || fragPosition.x >= centrePixelX + (spriteWidth/2)) {return INVALIDv2; /* Horizontally out of sprite bounds */}


	//yUV calculation.
	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2; /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);


	return vec2(xUV, 1.0f - yUV);
}


bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointB - pointA;
	float distToTargetSQ = dot(LOSDelta.xy, LOSDelta.xy);
	float distToTargetInv = inversesqrt(distToTargetSQ);
	vec3 LOSDirection = LOSDelta * distToTargetInv;
	Ray LOSRay = createRay(pointA.xy, LOSDirection.xy, 1.0f / distToTargetInv);


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



void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);	
	float fragDepth = imageLoad(renderedFrame, framePosition).a;


	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f);
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f);
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
	tanVerticalViewAngleOffset = tan(normY * (verticalFOV / 2.0f));
	zoomEffect = ((zoom) ? zoomFactor : 1.0f);


	vec2 closestUV = INVALIDv2;
	Sprite closestSprite;
	bool spriteHit = false;
	vec3 albedo;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	for (int index=0; index<numSprites; index++) {
		Sprite thisSprite = sprites[index];


		float spriteDistance = length(playerPosition.xy - thisSprite.position.xy);

		if (spriteDistance >= fragDepth || spriteDistance > maxRayDistance) {continue; /* Too far to see onscreen. */}


		vec2 spriteUV = getSpriteUV(thisSprite, thisSprite.centreX, spriteDistance);
		if (spriteUV == INVALIDv2) {continue; /* Invalid UV, from getSpriteUV() */}
		
		if (drawUV == 1) {
			albedo = vec3(spriteUV.xy, thisSprite.textureID/16);
			closestSprite = thisSprite;
		} else {
			//Linear, uses MM1 from minMipMapDistance and so on.
			float LODIndex = clamp((mipMapLevels * 2.0f / maxRayDistance) * (float(spriteDistance) - minMipMapDistance), 0.0, mipMapLevels); 
			vec4 alphaTexture = textureLod(textureArray, vec3(spriteUV.xy, float(thisSprite.textureID)), floor(LODIndex));
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
			spriteHit = true;
			closestSprite = thisSprite;
		}

		fragDepth = spriteDistance;
	}





	if (spriteHit) {
		if (noLighting) {
			fragColour = vec4(albedo.rgb, 1.0f);

		} else {
			vec3 realPosition3D = vec3(closestSprite.position.xy, closestSprite.position.z);
			for (int idx=0; idx<numLights; idx++) {
				Light thisLight = lights[idx];
				if (!thisLight.enabled) {continue;}

				bool inShadow = checkLOS(thisLight.position, closestSprite.position);
				if (!inShadow) {
					float distance = length(realPosition3D - thisLight.position);
					float brightness;
					if (distance < EPSILON) { //Sprite is on the light; probably serving as a visual marker for it.
						brightness = 1.0f;
					} else {
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance.
						float brightness = clamp(attenuation, 0.0f, 1.0f);
					}

					vec3 lightContribution = thisLight.colour * brightness;
					vec4 litColour = vec4(albedo.rgb * lightContribution, 1.0f);

					fragColour += litColour;
				}
			}

			if (headLampEnabled) {
				Light headLamp;
				headLamp.position = playerPosition;
				headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
				headLamp.intensity = 5.0f + (headLampFlicker / 768.0f); //headLampFlicker is 0-255.


				float distance = length(realPosition3D - headLamp.position);
				float attenuation = max(0.0, 1.0 - ((distance*distance) / (headLamp.intensity*headLamp.intensity))); //Intensity fades with distance to light.
				float brightness = clamp(attenuation, 0.0f, 1.0f);

				vec3 lightContribution = headLamp.colour * brightness;
				vec4 litColour = vec4(albedo.rgb * lightContribution, 1.0f);

				fragColour += litColour;
			}


			//Sun Effect
			bool sunBlocked = checkLOS(closestSprite.position, closestSprite.position + sunDirection * maxRayDistance);
			if (!sunBlocked) {
				vec4 litColour = vec4(albedo.rgb * sunColour.rgb, 1.0f);
				fragColour += litColour;
			}


			//Minimum/Maximum brightness.
			fragColour.rgb = clamp(fragColour.rgb, albedo * DEFAULT_BRIGHTNESS, albedo * 1.0f);
		}

		vec4 finalFragColour = vec4(fragColour.rgb, fragDepth);
		imageStore(renderedFrame, framePosition, finalFragColour);
	}
}