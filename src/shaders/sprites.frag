/* sprites.frag */
#version 460 core


uniform sampler2DArray textureArray;
uniform float playerViewAngle;
uniform vec3 playerPosition;
uniform bool zoom;
uniform int drawUV;
uniform bool headLampEnabled;


layout(rgba32f, binding = 0) uniform image2D renderedFrame;
layout(std140, binding = 10) uniform constUBO {
	float zoomFactor;
	float maxRayAngle;
	float maxRayDistance;

	vec2 textureSize;

	float padding[4];
};

struct Visplane {
	vec2 start;			//Visplane Start.
	vec2 end;			//Visplane End.
	float height;		//Visplane Height.
	int textureID;		//Visplane Texture.
	int valid;			//Visplane Validity.
	float _padding;		//Visplane Padding
};
layout(std430, binding = 2) buffer visplaneUBO {
	Visplane visplanes[64];
};

struct Wall {
	vec3 start;			//Wall Start.
	vec3 end;			//Wall End.
	int textureID;		//Wall Texture.
	int valid;			//Wall Validity.
	float _padding[2];	//Wall Padding.
};
layout(std430, binding = 3) buffer wallUBO {
	Wall walls[256];
};

struct Sprite {
	vec3 position;	//Sprite Position.
	float width;	//Sprite Width.
	int textureID;	//Sprite Texture ID.
	int valid;		//Sprite Validity.
	float _padding;	//Memory padding.
};
layout(std140, binding = 4) uniform spriteSSBO {
	Sprite sprites[32];
};

struct Light {
	vec3 position;		//Light Position.
	vec3 colour;		//Light Colour.
	float intensity;	//Light Intensity.
	int valid;			//Light Validity.
	float _padding;		//Light Padding.
};
layout(std140, binding = 5) uniform lightUBO {
	Light lights[64];
};

layout(std430, binding = 6) buffer depthBuffer {
	float depths[];
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
ivec2 renderResolution;
vec4 fragColour;
float fragDepth;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float DEFAULT_BRIGHTNESS = 0.25f;
const vec2 INVALID = vec2(1e30f, 1e30f);
const vec3 INVALIDv3 = vec3(1e30f, 1e30f, 1e30f);

const bool noLighting = false;


float determinant(vec2 vecA, vec2 vecB) {
	return (vecA.x * vecB.y) - (vecA.y * vecB.x);
}


vec2 rayIntersectCheck(Ray ray, Wall wall) {
	vec2 wallStartV2 = vec2(wall.start.x, wall.start.y);
	vec2 wallEndV2 = vec2(wall.end.x, wall.end.y);

	vec2 xDiff = vec2(ray.position.x - ray.end.x, wall.start.x - wall.end.x);
	vec2 yDiff = vec2(ray.position.y - ray.end.y, wall.start.y - wall.end.y);


	double divisor = determinant(xDiff, yDiff);
	//If less than some Epsilon value.
	if (abs(divisor) < 1e-5f) {
		//Lines do not intersect, as they are nearly parrallel.
		return INVALID;
	}


	vec2 dets = vec2(determinant(ray.position, ray.end), determinant(wallStartV2, wallEndV2));
	double xCoord = determinant(dets, xDiff) / divisor;
	double yCoord = determinant(dets, yDiff) / divisor;

	vec2 intersectPoint = vec2(xCoord, yCoord);


	//Check if the intersection is within the wall segment.
	if (intersectPoint.x < min(wall.start.x, wall.end.x) || intersectPoint.x > max(wall.start.x, wall.end.x) ||
		intersectPoint.y < min(wall.start.y, wall.end.y) || intersectPoint.y > max(wall.start.y, wall.end.y)) {
		return INVALID; // Intersection is outside the wall segment
	}


	vec2 intersectDirection = normalize(intersectPoint - ray.position);
	vec2 directionDifference = ray.direction - intersectDirection;

	
	if (length(directionDifference) < EPSILON_ALT) {
		//Wrong way, behind camera.
		return INVALID;
	}
	

	return intersectPoint;  
}

vec3 getVisplaneIntersect(Visplane plane, vec3 originPos, bool isLOSCheck=false, vec3 LOSDirection=vec3(0.0f, 0.0f, 0.0f)) {
	float targetZ = originPos.z - plane.height;
	vec2 position2D;

	if (isLOSCheck) { //Used in checkLOS().
		if (abs(LOSDirection.z) < EPSILON) {return INVALIDv3; /* Avoids DivZero error */}
		float t = targetZ / LOSDirection.z;
		if (t < 0.0f) {return INVALIDv3; /* Behind origin */}
		position2D = originPos.xy + LOSDirection.xy * t;


	} else { //Used within the main Visplane loop of main() for rendering.
		float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
		float rayOffset = -rayAngle + (fragPosition.x / renderResolution.x) * 2.0f * rayAngle;

		float verticalFOV = 2 * atan(tan(radians(rayAngle)) * (renderResolution.x / renderResolution.y));

		float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;
		float vAO = normY * (verticalFOV/2);
		float theta = radians(playerViewAngle + rayOffset);
		vec3 rayDirection = vec3(sin(theta), cos(theta), tan(vAO));


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
	}

	return vec3(position2D.xy, plane.height);
}


vec2 getSpriteUV(Sprite thisSprite, float centrePixelX, float depth) {
	const float spriteHeightUnits = 1.75f;
	float spriteFootZ = thisSprite.position.z - spriteHeightUnits/2.0f;
	float spriteHeadZ = thisSprite.position.z + spriteHeightUnits/2.0f;

	float zoomEffect = (zoom) ? zoomFactor : 1.0f;
	float distance = length(playerPosition.xy - thisSprite.position.xy) / zoomEffect;
	float projectedYLow = (playerPosition.z - spriteFootZ) / distance;
	float projectedYTop = (playerPosition.z - spriteHeadZ) / distance;

	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	float screenYTop = renderResolution.y * (0.5 - projectedYTop);


	float spriteHeight = screenYTop - screenYLow;
	float spriteWidth = thisSprite.width * spriteHeight;
	//spriteWidth = (zoom) ? spriteWidth * zoomFactor : spriteWidth;
	spriteHeight = (zoom) ? spriteHeight * zoomFactor : spriteHeight;



	//xUV calculation.
	float relativeX = fragPosition.x - centrePixelX + (spriteWidth/2);
	float xUV = fract(relativeX / spriteWidth);
	if (fragPosition.x < centrePixelX - (spriteWidth/2) || fragPosition.x >= centrePixelX + (spriteWidth/2)) {return INVALID; /* Horizontally out of sprite bounds */}


	//yUV calculation.
	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALID; /* Vertically out of sprite bounds */}
	float yUV = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);


	return vec2(xUV, 1.0f - yUV);
}



float angleClamp(float value) {
	if (value < 0.0f) {
		return angleClamp(360.0f + value);
	}
	return mod(value, 360.0f);
}


bool checkLOS(vec3 pointA, vec3 pointB, int thisIndex=-1, int foundType=0) {
	vec3 LOSDelta = pointA - pointB;
	float distToTargetSQ = dot(LOSDelta.xy, LOSDelta.xy);
	float distToTarget = sqrt(distToTargetSQ);
	vec3 LOSDirection = normalize(LOSDelta);
	Ray LOSRay = createRay(pointA.xy, normalize(LOSDelta.xy), distToTarget);


	//Iterate through all the walls. (2D)
	for (int idx = 0; idx < 256; idx++) {
		Wall thisWall = walls[idx];
		if (thisWall.valid <= 0 || (idx == thisIndex && foundType == 1)) {continue; /* Wall is empty or is the index calling the LOS check. */}

		vec2 intersectPoint = rayIntersectCheck(LOSRay, thisWall);
		if (intersectPoint == INVALID) {continue; /* Invalid intersect point */}

		float distToIntersectSQ = dot(pointA.xy - intersectPoint, pointA.xy - intersectPoint);
		if (distToIntersectSQ > distToTargetSQ) {continue; /* Not within the range of the LOScheck. */}

		float a = sqrt(distToIntersectSQ) / distToTarget;
		float actualZ = mix(pointA.z, pointB.z, a);

		if ((actualZ > min(thisWall.start.z, thisWall.end.z)) && (actualZ < max(thisWall.start.z, thisWall.end.z))) {
			return true;
		}
	}


	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<64; idx++) {
		Visplane thisPlane = visplanes[idx];
		if (thisPlane.valid <= 0 || (idx == thisIndex && foundType == 2)) {continue; /* Visplane is not valid or is the index calling the LOS check. */}
		if (thisPlane.height < min(pointA.z, pointB.z) || thisPlane.height > max(pointA.z, pointB.z)) {continue;}


		vec3 intersectPoint = getVisplaneIntersect(thisPlane, pointB, true, LOSDirection);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			continue;
		}
		return true;
	}

	return false;
}


float getSpriteScreenX(Sprite thisSprite, float rayAngle) {
	float f = tan(radians(rayAngle)); //tan(FOV/2)
	float a = radians(playerViewAngle);

	vec2 dir = vec2(sin(a), cos(a));
	vec2 plane = vec2(-cos(a) * f, sin(a) * f);
	vec2 spriteDir = thisSprite.position.xy - playerPosition.xy;

	if (dot(dir, normalize(spriteDir)) < 0.0f) {return 1e30f;}


	float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

	float transformX = invDet * (dir.y * spriteDir.x - dir.x * spriteDir.y);
	float transformY = invDet * (-plane.y * spriteDir.x + plane.x * spriteDir.y);

	return (renderResolution.x / 2.0f) * (1.0f - transformX / transformY);
}



void main() {
	fragPosition = gl_FragCoord.xy;
	renderResolution = imageSize(renderedFrame);
	ivec2 framePosition = ivec2(fragPosition);	
	float fragDepth = imageLoad(renderedFrame, framePosition).a;


	vec2 closestUV = INVALID;
	Sprite closestSprite;
	bool spriteHit = false;
	vec3 fragColour = vec3(0.0f, 0.0f, 0.0f);
	vec3 albedo;
	float rayAngle = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;

	for (int index = 0; index < 32; index++) {
		Sprite thisSprite = sprites[index];
		if (thisSprite.valid <= 0) {continue; /* Sprite is empty */}


		float spriteDistance = length(playerPosition.xy - thisSprite.position.xy);

		if (spriteDistance >= fragDepth || spriteDistance > maxRayDistance) {continue; /* Too far to see onscreen. */}



		float centrePixelX = getSpriteScreenX(thisSprite, rayAngle);
		if (centrePixelX == 1e30f) {continue; /* Invalid cpX, probably offscreen. */}


		vec2 spriteUV = getSpriteUV(thisSprite, centrePixelX, spriteDistance);
		if (spriteUV == INVALID) {continue; /* Invalid UV, from getSpriteUV() */}
		
		if (drawUV == 1) {
			albedo = vec3(spriteUV.xy, thisSprite.textureID/16);
			closestSprite = thisSprite;
		} else {
			vec4 alphaTexture = texture(textureArray, vec3(spriteUV.xy, float(thisSprite.textureID)));
			if (alphaTexture.a < 0.5f) {continue; /* This pixel is transparent. */}
			albedo = alphaTexture.rgb;
			spriteHit = true;
			closestSprite = thisSprite;
		}

		fragDepth = spriteDistance;
	}





	if (spriteHit) {
		if (noLighting) {
			fragColour = albedo.rgb;

		} else {
			for (int idx=0; idx<64; idx++) {
				Light thisLight = lights[idx];
				if (thisLight.valid <= 0) {continue; /* Light is not valid. */}

				bool shadow = checkLOS(thisLight.position, closestSprite.position);
				if (shadow) {
					fragColour = min(albedo.rgb * DEFAULT_BRIGHTNESS + fragColour.rgb, vec3(1.0f, 1.0f, 1.0f));
				} else {
					vec3 realPosition3D = vec3(closestSprite.position.xy, 1.0f);
					float distance = length(realPosition3D - thisLight.position);
					float brightness;
					if (distance < EPSILON) { //Sprite is on the light; probably serving as a visual marker for it.
						brightness = 1.0f;
					} else {
						float attenuation = max(0.0, 1.0 - ((distance*distance) / (thisLight.intensity*thisLight.intensity))); //Intensity fades with distance.
						brightness = clamp(attenuation, DEFAULT_BRIGHTNESS, 2.5);
					}

					vec3 lightContribution = thisLight.colour * brightness;
					vec3 litColor = albedo.rgb * lightContribution;

					fragColour = min(fragColour + litColor, vec3(1.0f, 1.0f, 1.0f));
				}
			}

			if (headLampEnabled) {
				Light headLamp;
				headLamp.position = playerPosition;
				headLamp.colour = vec3(1.0f, 1.0f, 1.0f);
				headLamp.intensity = 3.0f;
				headLamp.valid = 1;


				vec3 realPosition3D = vec3(closestSprite.position.xy, 1.0f);
				float distance = length(realPosition3D - headLamp.position);
				float attenuation = max(0.0, 1.0 - ((distance*distance) / (headLamp.intensity*headLamp.intensity))); //Intensity fades with distance to light.
				float brightness = clamp(attenuation, DEFAULT_BRIGHTNESS, 2.5);

				vec3 lightContribution = headLamp.colour * brightness;
				vec4 litColor = vec4(albedo.rgb * lightContribution, 1.0f);

				fragColour = min(litColor.rgb + fragColour.rgb, vec3(1.0f, 1.0f, 1.0f));
			}
		}

		vec4 finalFragColour = vec4(fragColour, fragDepth);
		imageStore(renderedFrame, framePosition, finalFragColour);
	}
}