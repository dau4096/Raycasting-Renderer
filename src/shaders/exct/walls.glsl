/* walls.glsl */
//Contains all wall visual processing funcs.



void writeNoWallIntersect(uint SSBOIndex) {
	WallIntersect noHit;
	noHit.distanceSQ = -1.0f; //Impossible depth means no hit.
	wallIntersects[SSBOIndex] = noHit;
}

float xUVFromWorldspace(vec2 intersectPoint, in Wall thisWall) {
	return mix(intersectPoint.x, intersectPoint.y, float(abs(thisWall.direction.y) > abs(thisWall.direction.x)));
}
float xUVFromWallspace(vec2 intersectPoint, Wall thisWall) {
	if (abs(thisWall.direction.y) > abs(thisWall.direction.x)) {
		return (intersectPoint.y - thisWall.start.y) / (thisWall.end.y - thisWall.start.y);
	}
	return (intersectPoint.x - thisWall.start.x) / (thisWall.end.x - thisWall.start.x);
}

uint getXUV(vec2 intersectPoint, in Wall thisWall) {
	//1st formatting data;
	bvec4 textureFlags;
	int textureID;
	unpackTextureFormattingBits1(
		thisWall.textureData1, textureFlags,
		textureID
	);

	//2nd formatting data;
	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits2(
		thisWall.textureData2, useWorldSpace,
		invTextureScale, textureOffset
	);



	//xUV calculation.
	float xUV;
	if (useWorldSpace.x) {
		xUV = xUVFromWorldspace(intersectPoint, thisWall);
	} else {
		xUV = xUVFromWallspace(intersectPoint, thisWall);
	}
	xUV *= invTextureScale.x;

	xUV = clamp(fract(xUV + textureOffset.x), 0.0f, 1.0f);
	return uint(xUV * 255.0f);
}



void unpackWallProjections(in Wall thisWall, uint projections, out float screenYLow, out float screenYTop) {
	//The ideal offset is -0x7FFF (-32,767), but they have slight offsets to account for floating-point inconsistencies later. (+/- 1px.)
	float lowOffset = (playerPosition.z < thisWall.start.z) ? 0.0f : -1.0f;
	screenYLow = float(int((projections >> 16) & 0xFFFFu) - 0x7FFF) + lowOffset;

	float topOffset = ((playerPosition.z > thisWall.end.z) ? 0.0f : 1.0f);
	screenYTop = float(int(projections & 0xFFFFu) - 0x7FFF) + topOffset;
}





float getWallYUVsingle(float z, float a, float invTextureScaleY, bool useWorldSpaceY, float textureOffsetY) {
	//return 1.0f - (mix(a, z, float(useWorldSpaceY)) * invTextureScaleY) + textureOffsetY;
	return (z * invTextureScaleY) + textureOffsetY;
}


void getWallYUVextremes(in Wall thisWall, uint projections, out float hUVy, out float lUVy, out uint numUVRepeats) {
	//Only get the uppermost/lowermost parts of the wall's yUV.
	//2nd formatting data;
	bvec2 useWorldSpace;
	vec2 invTextureScale;
	vec2 textureOffset;
	unpackTextureFormattingBits2(
		thisWall.textureData2, useWorldSpace,
		invTextureScale, textureOffset
	);

	//Somehow seems to depend on wall height, or possibly doesnt? everything tiles twice vertically, except the ones that don't inexplicably!?
	hUVy = getWallYUVsingle(thisWall.end.z, 1.0f, invTextureScale.y, useWorldSpace.y, textureOffset.y);
	lUVy = getWallYUVsingle(thisWall.start.z, 0.0f, invTextureScale.y, useWorldSpace.y, textureOffset.y);

	//Number of times the wall repeats in the range. Minimum 1, maximum 255.
	numUVRepeats = uint(ceil(abs(1.0f / invTextureScale.y)));
}