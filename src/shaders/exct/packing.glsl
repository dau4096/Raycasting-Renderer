/* packing.glsl */
//Generic functions for packing/unpacking data.



//Visplanes
uint encodeVPProjections(int higher, int lower) {
	uint biasedHigher = uint(higher + 0x7FFF) & 0xFFFF;
	uint biasedLower = uint(lower + 0x7FFF) & 0xFFFF;

	return (biasedHigher << 16) | biasedLower;
}
void unpackVPProjections(uint en, out int higher, out int lower) {
	higher = int(en >> 16) - 0x7FFF;
	lower = int(en & 0xFFFF) - 0x7FFF;
}




//Walls
uint getProjectedZ(float wallDistanceSQ, float wallLowZ, float wallTopZ) {
	float invDistance = inversesqrt(wallDistanceSQ) * zoomEffect * aspectRatio;
	//Inverse distance scaled by zooming and a set multiplier to make 1x1u more square (was 2:3 ratio before.)

	//Low edge of the wall (bottom visually)
	float projectedYLow = (playerPosition.z - wallLowZ) * invDistance;
	float screenYLow = renderResolution.y * (0.5f - projectedYLow);
	uint clampedScreenYLow = uint(clamp(int(screenYLow) + 0x7FFF, 0x0000, 0xFFFF));

	//High edge of the wall (top visually)
	float projectedYTop = (playerPosition.z - wallTopZ) * invDistance;
	float screenYTop = renderResolution.y * (0.5f - projectedYTop);
	uint clampedScreenYTop = uint(clamp(int(screenYTop) + 0x7FFF, 0x0000, 0xFFFF));

	//Convert to 16-bit unsigned integers and return combined projection.
	return (clampedScreenYLow << 16) | clampedScreenYTop;
}




//Texture UV
#define UV_SCALE_CONSTANT 65535.0f /* (float)(2^16 - 1) */
uint getInvUV(float value) {
	uint en = uint(value * UV_SCALE_CONSTANT + 0.5f); //Scale up and round to an int.
	return en & 0xFFFF;
}
float unpackInvUV(uint en) {
	return float(en) / UV_SCALE_CONSTANT; //Scale back down, convert to float.
}


//Walls
uint encodeWallTextureUVs(float hUVy, float lUVy) {
	return uint((getInvUV(hUVy) << 16) | getInvUV(lUVy));
}

void unpackWallTextureUVs(uint en, out float hUVy, out float lUVy) {
	hUVy = unpackInvUV(en >> 16);
	lUVy = unpackInvUV(en & 0xFFFF);
}


//Visplanes
uvec2 encodeVPTextureUVs(vec2 hUV, vec2 lUV) {
	return uvec2(
		(getInvUV(hUV.x) << 16) | getInvUV(hUV.y), //Upper UVs
		(getInvUV(lUV.x) << 16) | getInvUV(lUV.y)  //Lower UVs
	);
}

void unpackVPTextureUVs(uvec2 en, out vec2 hUV, out vec2 lUV) {
	hUV = vec2(unpackInvUV(en.x >> 16), unpackInvUV(en.x & 0xFFFF));
	lUV = vec2(unpackInvUV(en.y >> 16), unpackInvUV(en.y & 0xFFFF));
}
