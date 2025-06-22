/* environment.frag */
#version 460 core


//Samplers
layout(binding=0) uniform sampler2DArray textureArray;
layout(binding=1) uniform sampler2D skyboxTexture;

//CameraData
uniform float maxRayDistance;
uniform float maxRayAngle;
uniform float verticalFOV;
uniform float zoomFactor;
uniform bool zoom;
uniform vec2 textureScale;
uniform vec3 textureOffset;
uniform bool useMipMapping;

//PlayerData
uniform float playerViewAngle;
uniform float playerViewRoll;
uniform float playerViewPitch;
uniform vec3 playerPosition;
uniform ivec2 renderResolution;

//Debug
uniform int debugMode;

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
uniform int numLights;
uniform float shadowMapQuality;
uniform bool allowTransparency;


layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(rgba32f, binding=1) uniform image2D positionMap;
layout(rgba32f, binding=2) uniform image2D normalMap;


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



struct IntersectionData {
	vec3 position;
	vec3 UV;
	double distanceSQ;
	int index, foundType;
};
IntersectionData stack[3];
int topOfStack = 0;

void pushStack(IntersectionData data) {
	int insertIdx = topOfStack;

	for (int i=0; i<topOfStack; i++) {
		if (data.distanceSQ < stack[i].distanceSQ) {
			insertIdx = i;
			break;
		}
	}

	if (topOfStack < 3) {
		topOfStack++;
	}
	for (int i=topOfStack-1; i>insertIdx; i--) {
		stack[i] = stack[i-1];
	}

	stack[insertIdx] = data;
}


bool popStack(out IntersectionData data) {
	if (topOfStack > 0) {
		data = stack[0];
		for (int i=0; i<topOfStack - 1; i++) {
			stack[i] = stack[i+1];
		}
		topOfStack--;
		return true;
	}
	return false;
}




//Ray Struct.
struct Ray {
	dvec2 position, end;
	dvec2 direction;
};

Ray createRay(dvec2 position, dvec2 direction, double maxDist=maxRayDistance) {
	Ray ray;
	ray.position = position;
	ray.direction = direction;
	ray.end = ray.position + (ray.direction.xy * maxDist);
	return ray;
};


vec2 fragPosition;
vec2 rayDirectionCentre;
vec4 fragColour;
float zoomEffect;
float halfFOV;
double t, fragZ;
const float INF = 0xFFFFFF;
const float EPSILON = 1e-4f;
const float EPSILON_ALT = 1e-3f;
const float MIN_WALL_DIST = 0.125f;
const dvec2 INVALIDdv2 = dvec2(INF, INF);
const vec2 INVALIDv2 = vec2(INF, INF);
const vec3 INVALIDv3 = vec3(INF, INF, INF);
const vec4 INVALIDv4 = vec4(INF, INF, INF, INF);

const bool blendMipMap = true;
const bool forceMipMapLevel = false;
const float forcedMipMapLevel = 0.0f;
const bool debugMipMapLevel = false;

const float mipMapLevels = 7.0f;
const float minMipMapDistance = 5.0f;






vec4 fetchUV(vec3 UV, double distance, vec3 surfaceNormal, vec3 surfacePosition, bool fetchTexture=true) {
	if (debugMode == 1) {
		return vec4(UV.xy, UV.z / 32.0f, maxRayDistance);
	}
	if (!fetchTexture || (UV.z < 0)) return vec4(1.0f, 0.0f, 1.0f, 1.0f);
	if (!useMipMapping) {
		return textureLod(textureArray, UV, 0.0f);
	}
	if (forceMipMapLevel) {
		return textureLod(textureArray, UV, forcedMipMapLevel);
	}

	//Linear, uses MM1 from minMipMapDistance and so on.
	float depthComponent = (mipMapLevels * 2.0f / maxRayDistance) * (float(distance) - minMipMapDistance);
	//Walls facing camera appear sharper, visplanes at grazing angles appear smoother etc.
	float slopeComponent = -abs(dot(normalize(playerPosition - surfacePosition), surfaceNormal));
	float LODIndex = clamp(depthComponent + slopeComponent, 0.0, mipMapLevels); 

	if (debugMipMapLevel) {
		return vec4(LODIndex / mipMapLevels, fract(LODIndex), 0.0f, 1.0f);
	}
	vec4 mipMapColour = textureLod(textureArray, UV, ceil(LODIndex));
	if (!blendMipMap) {
		return mipMapColour;
	}
	vec4 MipMapMinusOneColour = textureLod(textureArray, UV, ceil(LODIndex) - 1.0f);
	return mix(MipMapMinusOneColour, mipMapColour, fract(LODIndex));
}




//GLSL Cross only works on vec3.
double cross2D(dvec2 a, dvec2 b) {
	return a.x * b.y - a.y * b.x;
}
float cross2D(vec2 a, vec2 b) {
	return a.x * b.y - a.y * b.x;
}






//Walls
vec2 getWallUV(Wall thisWall, dvec2 intersectPoint, vec3 originPos) {
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//xUV calculation.
	double xUV;
	if (abs(thisWall.direction.y) > abs(thisWall.direction.x)) {
		xUV = fract(intersectPoint.y / textureScale.x);
	} else {
		xUV = fract(intersectPoint.x / textureScale.x);
	}
	if (xUV < 0.0f) {xUV = 1.0 - abs(xUV);}


	//yUV calculation.
	double distance = length(originPos.xy - intersectPoint) / zoomEffect;
	double projectedYLow = (originPos.z - wallLowZ) / distance;
	double projectedYTop = (originPos.z - wallTopZ) / distance;

	double screenYLow = renderResolution.y * (0.5 - projectedYLow);
	double screenYTop = renderResolution.y * (0.5 - projectedYTop);

	if (fragPosition.y > screenYTop || fragPosition.y < screenYLow) {return INVALIDv2;}

	double a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow); //Alpha to mix by.
	fragZ = mix(wallLowZ, wallTopZ, a);
	double yUV = 1.0f - fract(fragZ / textureScale.y);


	return vec2(xUV, yUV) + textureOffset.xz;
}








//Displacements
vec2 getScreenPosition(vec3 position3D) {
	vec2 delta = position3D.xy - playerPosition.xy;

	//X
	vec2 direction = normalize(delta);
	float theta = atan(direction.x, direction.y);
	float rayDelta = degrees(theta) - playerViewAngle;
	if (rayDelta > 180.0f) rayDelta -= 360.0f;
	if (rayDelta < -180.0f) rayDelta += 360.0f;
	float X = (renderResolution.x / 2.0f) * ((rayDelta / halfFOV) + 1.0f);

	//Y
	/*
	//Original from getWallUV()
	float projectedYTop = (originPos.z - wallTopZ) / distance;
	float screenYLow = renderResolution.y * (0.5 - projectedYLow);
	*/

	float invdistance = inversesqrt(max(dot(delta, delta), 1e-4f));
	float projCentreY = (playerPosition.z - position3D.z) * invdistance * zoomEffect;
	float Y = renderResolution.y * (0.5f - projCentreY);

	return vec2(X, Y);
}

float sign2(vec2 a, vec2 b, vec2 c) {
	return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
}
float edge(vec2 a, vec2 b, vec2 c) {
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

bool inDisplacement(vec2 v1, vec2 v2, vec2 v3) {
	float d1 = sign2(fragPosition, v1, v2);
	float d2 = sign2(fragPosition, v2, v3);
	float d3 = sign2(fragPosition, v3, v1);

	bool hasNegative = (d1 < 0) || (d2 < 0) || (d3 < 0);
	bool hasPositive = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(hasNegative && hasPositive);
}

vec3 barycentricWeights(vec2 v1, vec2 v2, vec2 v3) {
	float areaABC = edge(v1, v2, v3);
	float a = edge(fragPosition, v2, v3)/areaABC;
	float b = edge(fragPosition, v3, v1)/areaABC;
	float c = 1.0f - a - b;
	return vec3(a,b,c);
}

bool behindCamera(vec4 pt, vec2 proj) {
	vec2 dir = normalize(pt.xy - playerPosition.xy);
	return (dot(rayDirectionCentre, dir) < 0.0f) || (proj.x < 0.0f || proj.x > renderResolution.x);
}



//Visplanes
vec3 getVisplaneIntersect(Visplane plane, vec3 originPos, vec2 rayDirection) {
	float targetZ = (originPos.z - plane.height) * zoomEffect;
	vec2 position2D;

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

vec2 getVisplaneUV(vec3 position3D) {
	bool topHalf = fragPosition.y > renderResolution.y/2;
	vec2 realPosition = position3D.xy;

	//Take the fractional parts of the position (texture tiles every unit square)
	float xUV = fract(realPosition.x / textureScale.x);
	if (xUV < 0.0f) {xUV = 1.0f - abs(xUV);}
	if (!topHalf) {
		xUV = 1.0f - xUV;
	}
	float yUV = fract(realPosition.y / textureScale.y);
	if (yUV < 0.0f) {yUV = 1.0f - abs(yUV);}
	//Texture index depends on top (ceiling) or bottom (floor) half.

	return vec2(xUV, yUV) + textureOffset.xy;
}





void main() {
	fragPosition = gl_FragCoord.xy;
	ivec2 framePosition = ivec2(fragPosition);
	fragColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	bool shouldDrawToPositionMap = (framePosition.x % int(shadowMapQuality) == 0) && (framePosition.y % int(shadowMapQuality) == 0);


	zoomEffect = ((zoom) ? zoomFactor : 1.0f);
	//Negative is upward; so subtract.
	float rollDecimal = clamp(playerViewRoll / 22.5f, -1.0f, 1.0f) * zoomEffect;
	fragPosition.y -= (fragPosition.x - renderResolution.x / 2.0f) * rollDecimal;
	float pitchDecimal = clamp(playerViewPitch, -22.5f, 22.5f) * zoomEffect;
	fragPosition.y -= (pitchDecimal * renderResolution.y) / 54.0f; //Scaling to resolution. 10px per degree if it's 540px tall.
	bool lowerHalf = fragPosition.y < (renderResolution.y / 2.0f); //If the frag has no possible way to intersect a visplane below (or above) then skip those.


	halfFOV = (zoom) ? maxRayAngle / zoomFactor : maxRayAngle;
	float rayOffset = -halfFOV + (fragPosition.x / renderResolution.x) * 2.0f * halfFOV;
	float rayAngleYaw = radians(playerViewAngle + rayOffset);

	vec2 rayDirection = vec2(sin(rayAngleYaw), cos(rayAngleYaw));
	float rPVA = radians(playerViewAngle);
	rayDirectionCentre = vec2(sin(rPVA), cos(rPVA));
	float distMultiplier = dot(rayDirection, rayDirectionCentre);

	Ray fragRay = createRay(playerPosition.xy, rayDirection);

	vec2 rayStart = playerPosition.xy;
	vec2 rayEnd = vec2(fragRay.end.xy);
	
	float normY = (2.0 * fragPosition.y / renderResolution.y) - 1.0;








	//Find closest intersect
	IntersectionData thisIntersect;
	thisIntersect.distanceSQ = maxRayDistance * maxRayDistance;
	bool foundObject = false;

	//Iterate through all the walls. (2D)
	for (int idx=0; idx<numWalls; idx++) {
		Wall thisWall = walls[idx];
		vec2 wallNormal = vec2(-thisWall.direction.y, thisWall.direction.x);
		float projStart = dot(rayStart-thisWall.start.xy, wallNormal);
		float projEnd = dot(rayEnd-thisWall.start.xy, wallNormal);
		if (projStart * projEnd >= 0.0f) {continue; /* Ray never crosses wall. */}

		double t = (projStart) / (projEnd - projStart);
		dvec2 intersectPoint = playerPosition.xy - rayDirection.xy * t * maxRayDistance;
		vec2 minWall = min(thisWall.start.xy, thisWall.end.xy);
		vec2 maxWall = max(thisWall.start.xy, thisWall.end.xy);
		if (
		    intersectPoint.x + EPSILON_ALT < minWall.x || intersectPoint.x - EPSILON_ALT > maxWall.x ||
		    intersectPoint.y + EPSILON_ALT < minWall.y || intersectPoint.y - EPSILON_ALT > maxWall.y
		) {
			//Outside of valid wall segment.
			continue;
		}

		dvec3 intersectPointv3 = dvec3(intersectPoint.xy, playerPosition.z);
		double wallDistanceSQ = dot(playerPosition - intersectPointv3, playerPosition - intersectPointv3); //Cheaper length() call

		vec2 wallUV = getWallUV(thisWall, intersectPoint, playerPosition); //Check if inside wall (Valid UV)


		if (wallUV != INVALIDv2) {
			//Set closest.
			thisIntersect.distanceSQ = wallDistanceSQ;
			thisIntersect.index = idx;
			thisIntersect.position = vec3(intersectPoint.xy, fragZ);
			thisIntersect.UV = vec3(wallUV.xy, thisWall.textureID);
			thisIntersect.foundType = 1;
			pushStack(thisIntersect);
			foundObject = true;
		}
		
	}

	//Iterate through all visplanes. (3D)
	for (int idx=0; idx<numVisplanes; idx++) {
		Visplane thisPlane = visplanes[idx];

		if (
			(lowerHalf && thisPlane.height > playerPosition.z) ||
			(!lowerHalf && thisPlane.height < playerPosition.z)
		) {
			//Fragray cannot possibly hit visplane.
			continue;
		}

		vec3 intersectPoint = getVisplaneIntersect(thisPlane, playerPosition, rayDirection);
		if (intersectPoint == INVALIDv3) {continue; /* Invalid Intersect */}
		if ((intersectPoint.x < min(thisPlane.start.x, thisPlane.end.x)) || (intersectPoint.x > max(thisPlane.start.x, thisPlane.end.x)) ||
			(intersectPoint.y < min(thisPlane.start.y, thisPlane.end.y)) || (intersectPoint.y > max(thisPlane.start.y, thisPlane.end.y))) {
			//Out of the range of the Visplane.
			continue;
		}

		float vPlaneDistanceSQ = dot(playerPosition.xy - intersectPoint.xy, playerPosition.xy - intersectPoint.xy); //Cheaper length() call
		thisIntersect.distanceSQ = vPlaneDistanceSQ;
		thisIntersect.position = intersectPoint;
		thisIntersect.index = idx;
		thisIntersect.foundType = 2;
		thisIntersect.UV = vec3(getVisplaneUV(intersectPoint), thisPlane.textureID);
		pushStack(thisIntersect);
		foundObject = true;
	}


	//Iterate through all displacements. (pseudo-3D)
	for (int idx=0; idx<numDisplacements; idx++) {
		Displacement thisDisp = displacements[idx];

		vec2 pA = getScreenPosition(thisDisp.vertices[0].xyz);
		vec2 pB = getScreenPosition(thisDisp.vertices[1].xyz);
		vec2 pC = getScreenPosition(thisDisp.vertices[2].xyz);

		if (behindCamera(thisDisp.vertices[0], pA) && behindCamera(thisDisp.vertices[1], pB) && behindCamera(thisDisp.vertices[2], pC)) {continue;}

		if (inDisplacement(pA, pB, pC)) {
			vec3 barycentricW = barycentricWeights(pA, pB, pC);
			vec3 pos3D = (
				thisDisp.vertices[0].xyz * barycentricW.x +
				thisDisp.vertices[1].xyz * barycentricW.y +
				thisDisp.vertices[2].xyz * barycentricW.z
			);
			vec2 delta = pos3D.xy - playerPosition.xy;
			float distanceSQ = dot(delta, delta);

			
			thisIntersect.distanceSQ = distanceSQ;
			thisIntersect.index = idx;
			thisIntersect.position = pos3D;
			thisIntersect.foundType = 3;

			vec2 UV = (
				thisDisp.UV[0].xy * barycentricW.x +
				thisDisp.UV[1].xy * barycentricW.y +
				thisDisp.UV[2].xy * barycentricW.z
			);
			thisIntersect.UV = vec3(UV.xy, thisDisp.normal_texID.w);
			pushStack(thisIntersect);
			foundObject = true;
		}
	}


	
	



	if (foundObject) { //An intersect was found.
		IntersectionData validIntersect;
		bool success = true;
		vec3 normal;
		int typeFlag;
		vec4 albedo;
		double minDistance;
		bool trueFound = false;
		while (success) {
			success = popStack(validIntersect);
			
			Wall closestWall;
			Visplane closestPlane;
			Displacement closestDisp;
			if (validIntersect.foundType == 1) { //Wall
				closestWall = walls[validIntersect.index];

				vec2 wallDirection = normalize(closestWall.end - closestWall.start).xy;
				vec2 normalv2 = vec2(wallDirection.y, -wallDirection.x);
				if (dot(normalv2, playerPosition.xy - validIntersect.position.xy) < 0.0) {
					normalv2 *= -1;
					validIntersect.UV.x *= -1;
				}
				normal = vec3(normalv2.xy, 0.0f);
				typeFlag = 0x1;

			} else if (validIntersect.foundType == 2) { //Visplane
				closestPlane = visplanes[validIntersect.index];

				if (closestPlane.height > playerPosition.z) {
					normal = vec3(0.0f, 0.0f, -1.0f);
				} else {
					normal = vec3(0.0f, 0.0f, 1.0f);
				}
				typeFlag = 0x2;

			} else if (validIntersect.foundType == 3) {
				closestDisp = displacements[validIntersect.index];
				normal = closestDisp.normal_texID.xyz;
				typeFlag = 0x3;
			}

			minDistance = distMultiplier / inversesqrt(validIntersect.distanceSQ);
			albedo = fetchUV(validIntersect.UV, minDistance, normal, validIntersect.position);
			if ((albedo.a < 0.5f) && (allowTransparency)) {continue;}
			if (shouldDrawToPositionMap) {
				int idx = (validIntersect.index << 2) | typeFlag;
				ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
				imageStore(positionMap, thisFramePosition, vec4(validIntersect.position, float(idx)));
				imageStore(normalMap, thisFramePosition, vec4(normalize(normal.xyz), 1.0f));
			}
			trueFound = true;
			break;

		}

		if (trueFound) {
			if (debugMode == 2) { //Drawing normals.
				fragColour = vec4((normal.xyz * 0.5f) + 0.5f, 1.0f);
			} else {
				fragColour = albedo;
			}
			vec4 finalFragColour = vec4(fragColour.rgb, minDistance);
			imageStore(renderedFrame, framePosition, finalFragColour);
			return;
		}
	}

	vec2 UV = vec2(
		fract(rayAngleYaw / 6.28318530718f), //Over 2*Pi.
		1.0f - ((normY + 1.0f) / 2.0f) //Invert Y coordinate.
	);
	vec3 skyAlbedo = texture(skyboxTexture, UV).rgb;
	if (shouldDrawToPositionMap) {
		ivec2 thisFramePosition = ivec2(gl_FragCoord.xy / shadowMapQuality);
		imageStore(positionMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 0.0f));
		imageStore(normalMap, thisFramePosition, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	}
	imageStore(renderedFrame, framePosition, vec4(skyAlbedo.rgb, maxRayDistance));
}