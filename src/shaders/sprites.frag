/* sprites.frag */
#version 460 core


//Samplers
layout(binding = 0) uniform sampler2DArray textureArray;
layout(binding = 1) uniform sampler2D playerTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;
uniform int recursionIdx;

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
uniform int numWalls;
uniform int numVisplanes;
uniform int numSprites;
uniform int numLights;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(rgba32f, binding = 1) uniform image2D portalMask;
layout(rgba32f, binding = 2) uniform image2D portalMaskTMP;


struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std140, binding = 7) uniform visplaneUBO {
	Visplane visplanes[128];
};

struct Wall {
	vec3 start;        float _pad0;
	vec3 end;          float _pad1;
	vec2 direction;    vec2 _pad2;
	int textureID;     int type;
	float extra;       int valid;
	vec2 _padding;     vec2 _pad3;
};
layout(std140, binding = 3) uniform wallUBO {
	Wall walls[512];
};

struct Sprite {
	vec3 position;	//Sprite Position.
	float width;	//Sprite Width.
	float height;	//Sprite Height.
	int textureID;	//Sprite Texture ID.
	int valid;		//Sprite Validity.
};
layout(std140, binding = 4) uniform spriteUBO {
	Sprite sprites[64];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 5) uniform lightUBO {
	Light lights[128];
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


bool throughPortal, inPortal;
vec3 camPosition;
float camViewAngle;
vec2 fragPosition;
vec4 fragColour;
float fragDepth, tanVerticalViewAngleOffset, zoomEffect;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.25f;
const float MIN_WALL_DIST = 0.125f;
const float HEADLAMP_MIN_LIGHT = 0.1f;
const vec2 INVALIDv2 = vec2(1e30f, 1e30f);
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec3 INVALIDv3 = vec3(1e30f, 1e30f, 1e30f);

const bool noLighting = false;



dvec2 rayIntersectCheck(Ray ray, Wall wall) {
	dvec2 wallStartV2 = wall.start.xy;
	dvec2 wallEndV2 = wall.end.xy;

	dvec2 r = ray.end - ray.position;
	dvec2 s = wallEndV2 - wallStartV2;

	double denom = r.x * s.y - r.y * s.x;
	if (abs(denom) < EPSILON) return INVALIDdv2;

	dvec2 diff = wallStartV2 - ray.position;
	double t = (diff.x * s.y - diff.y * s.x) / denom;
	double u = (diff.x * r.y - diff.y * r.x) / denom;

	if (t < 0.0f || u < 0.0f || u > 1.0f) return INVALIDdv2;

	return ray.position + t * r;
}



bool wallIntersectQuick(vec3 start, vec3 end, Wall wall) {
	vec2 wallDirection = normalize(wall.start.xy - wall.end.xy);
	vec2 wallNormal = vec2(-wallDirection.y, wallDirection.x);

	float startProj = dot(start.xy - wall.start.xy, wallNormal);
	float endProj = dot(end.xy - wall.start.xy, wallNormal);
	if (startProj * endProj >= 0.0f) {return false;}
	if (sign(startProj) == sign(endProj)) {return false;}

	float denom = endProj - startProj;
	if (abs(denom) < EPSILON) return false;
	float alpha = startProj / denom;

	if (alpha < 0.0f || alpha > 1.0f) {return false;}

	float thisZ = mix(start.z, end.z, alpha);
	return (thisZ >= wall.start.z - EPSILON) && (thisZ <= wall.end.z + EPSILON);
}


vec3 getVisplaneIntersect(Visplane plane, vec3 originPos) {
	float targetZ = (originPos.z - plane.height) * zoomEffect;
	vec2 position2D;

	float halfFOV = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -halfFOV + (fragPosition.x / renderResolution.x) * 2.0f * halfFOV;

	float theta = radians(camViewAngle + rayOffset);
	vec3 rayDirection = vec3(sin(theta), cos(theta), tanVerticalViewAngleOffset);


	/*
	//Original from getWallUV()
	float projectedYTop = (originPos.z - wallTopZ) / distance;
	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	*/

	float antiProjection = 0.5f - (fragPosition.y/renderResolution.y);
	if (abs(antiProjection) < EPSILON) {return INVALIDv3; /* Avoids DivZero error */}
	float t = targetZ / antiProjection;
	if (t < 0.0f) {return INVALIDv3; /* Behind origin */}
	position2D = originPos.xy + rayDirection.xy * t;

	return vec3(position2D.xy, plane.height);
}


vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth) {
	float spriteFootZ = thisSprite.position.z - thisSprite.height/2.0f;
	float spriteHeadZ = thisSprite.position.z + thisSprite.height/2.0f;

	float zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float distance = length(camPosition.xy - thisSprite.position.xy) / zoomEffect;
	float projectedYLow = (camPosition.z - spriteFootZ) / distance;
	float projectedYTop = (camPosition.z - spriteHeadZ) / distance;

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
	float distToTarget = sqrt(distToTargetSQ);
	dvec3 LOSDirection = normalize(LOSDelta);
	Ray LOSRay = createRay(pointA.xy, normalize(LOSDelta.xy), distToTarget);


	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		if (idx == thisIndex && foundType == 1) {continue; /* Wall is empty or is the index calling the LOS check. */}

		bool intersect = wallIntersectQuick(pointA, pointB, thisWall);
		if (intersect) {return true;}
	}


	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<numVisplanes; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (idx == thisIndex && foundType == 2) {continue; /* Visplane is not valid or is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		double tFrac = (thisPlane.height - pointA.z) / LOSDelta.z;
		dvec3 intersectPoint = pointA + LOSDirection * tFrac;
		if ((intersectPoint.x > min(thisPlane.start.x, thisPlane.end.x)) && (intersectPoint.x < max(thisPlane.start.x, thisPlane.end.x)) &&
			(intersectPoint.y > min(thisPlane.start.y, thisPlane.end.y)) && (intersectPoint.y < max(thisPlane.start.y, thisPlane.end.y))) {
			return true;
		}
	}

	return false;
}


float getSpriteScreenX(Sprite thisSprite, float rayAngle) {
	float f = tan(radians(rayAngle)); //tan(FOV/2)
	float a = radians(camViewAngle);

	vec2 dir = vec2(sin(a), cos(a));
	vec2 plane = vec2(-cos(a) * f, sin(a) * f);
	vec2 spriteDir = thisSprite.position.xy - camPosition.xy;

	if (dot(dir, normalize(spriteDir)) < 0.0f) {return INF;}


	float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

	float transformX = invDet * (dir.y * spriteDir.x - dir.x * spriteDir.y);
	float transformY = invDet * (-plane.y * spriteDir.x + plane.x * spriteDir.y);

	return (renderResolution.x / 2.0f) * (1.0f - transformX / transformY);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	float fragDepth = imageLoad(renderedFrame, framePosition).a;


	float baseDistance = 0.0f;
	float nearDistance = MIN_WALL_DIST;


	camViewAngle = playerViewAngle;
	camPosition = playerPosition;


	inPortal = recursionIdx > 0;
	if (inPortal) { //Uses mask to draw area through portals.
		vec4 maskData = imageLoad(portalMaskTMP, framePosition);
		ivec2 portalMaskIndices = ivec2(maskData.xy);
		//fragDepth = imageLoad(portalMask, framePosition).z;
		baseDistance = maskData.z;
		nearDistance = baseDistance;
		if (portalMaskIndices.x == portalMaskIndices.y) {return;}

		Wall portalIn = walls[portalMaskIndices.x];
		Wall portalOut = walls[portalMaskIndices.y];

		float inAngle = atan(portalIn.direction.y, portalIn.direction.x);
		float outAngle = atan(portalOut.direction.y, portalOut.direction.x);
		float camAngle = radians(playerViewAngle);
		float delta = camAngle - inAngle;
		camViewAngle = degrees(outAngle + delta);

		float portalAngleDelta = outAngle - inAngle;
		float cosA = cos(portalAngleDelta);
		float sinA = sin(portalAngleDelta);

		vec2 rel = playerPosition.xy - portalIn.start.xy;
		vec2 rotatedRel;
		rotatedRel.x = rel.x * cosA - rel.y * sinA;
		rotatedRel.y = rel.x * sinA + rel.y * cosA;

		float dZ = playerPosition.z - portalIn.start.z;
		float newZ = portalOut.start.z + dZ;

		camPosition = vec3(portalOut.start.xy + rotatedRel, newZ);

	}



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

		float spriteDistance = length(thisSprite.position.xy - camPosition.xy);
		if (spriteDistance >= fragDepth || spriteDistance <= nearDistance || spriteDistance > maxRayDistance) {continue; /* Too far to see onscreen. */}

		float centrePixelX = getSpriteScreenX(thisSprite, rayAngle);
		if (centrePixelX == INF) {continue; /* Invalid cpX, probably offscreen. */}

		vec2 spriteUV = getSpriteUV(thisSprite, centrePixelX, spriteDistance);
		if (spriteUV == INVALIDv2) {continue; /* Invalid UV, from getSpriteUV() */}
		
		if (drawUV > 0) {
			albedo = vec3(spriteUV.xy, thisSprite.textureID/16);
			closestSprite = thisSprite;
			fragDepth = spriteDistance;
		} else {
			vec4 alphaTexture = texture(textureArray, vec3(spriteUV.xy, float(thisSprite.textureID)));
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
			spriteHit = true;
			closestSprite = thisSprite;
			fragDepth = spriteDistance;
		}

	}



	if (spriteHit) {
		if (noLighting) {
			fragColour = vec4(albedo.rgb, 1.0f);

		} else {
			vec3 realPosition3D = vec3(closestSprite.position.xy, closestSprite.position.z);
			for (int idx=0; idx<numLights; idx++) {
				Light thisLight = lights[idx];

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
				headLamp.position = camPosition;
				headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
				headLamp.intensity = 5.0f + (headLampFlicker / 768.0f); //headLampFlicker is 0-255.
				headLamp.valid = 1;


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


		vec4 finalFragColour = vec4(fragColour.rgb, fragDepth + baseDistance);
		imageStore(renderedFrame, framePosition, finalFragColour);
		imageStore(portalMask, framePosition, vec4(-1.0f, -1.0f, 0.0f, 0.0f)); //Blocked portal.
	}
}