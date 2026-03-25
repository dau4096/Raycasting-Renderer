/* walls.glsl */
//Contains all wall visual processing funcs.

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





float getWallYUV(Wall thisWall, uint projections, out int textureID, out bvec4 textureFlags, vec2 fragPosition, out float fragZ) {
	float wallLowZ = thisWall.start.z, wallTopZ = thisWall.end.z;

	//1st formatting data;
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


	//The ideal offset is -0x7FFF (-32,767), but they have slight offsets to account for floating-point inconsistencies later. (+/- 1px.)
	float lowOffset = (playerPosition.z < thisWall.start.z) ? 0.0f : -1.0f;
	float screenYLow = float(int(projections & 0xFFFFu) - 0x7FFF) + lowOffset;

	float topOffset = ((playerPosition.z > thisWall.end.z) ? 0.0f : 1.0f);
	float screenYTop = float(int((projections >> 16) & 0xFFFFu) - 0x7FFF) + topOffset;

	if (fragPosition.y >= screenYTop || fragPosition.y <= screenYLow) {
		return INF;
	}

	float a = (fragPosition.y - screenYLow) / (screenYTop - screenYLow);
	fragZ = mix(wallLowZ, wallTopZ, a);

	float yUVWorld = 1.0f - fragZ * invTextureScale.y;
	float yUVLocal = 1.0f - (a * invTextureScale.y);

	float yUV = mix(
		1.0f - (a * invTextureScale.y),   //Local
		1.0f - fragZ * invTextureScale.y, //World
		float(useWorldSpace.y)
	);

	return yUV + textureOffset.y;
}