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
uint getProjectedZ(
	float wallDistanceSQ,
	float wallLowZ, float wallTopZ,
	float cameraZ, uint minmaxY
) {
	float invDistance = inversesqrt(wallDistanceSQ) * zoomEffect * aspectRatio;
	//Inverse distance scaled by zooming and a set multiplier to make 1x1u more square (was 2:3 ratio before.)

	uint maxY = minmaxY >> 16u;
	uint minY = minmaxY & 0xFFFFu;

	//Low edge of the wall (bottom visually)
	float projectedYLow = (cameraZ - wallLowZ) * invDistance;
	float screenYLow = renderResolution.y * (0.5f - projectedYLow);
	uint clampedScreenYLow = uint(clamp(int(screenYLow) + 0x7FFF, minY, maxY));

	//High edge of the wall (top visually)
	float projectedYTop = (cameraZ - wallTopZ) * invDistance;
	float screenYTop = renderResolution.y * (0.5f - projectedYTop);
	uint clampedScreenYTop = uint(clamp(int(screenYTop) + 0x7FFF, minY, maxY));

	if (clampedScreenYLow >= clampedScreenYTop) {
	    return 0xFFFF0000u;
	}

	//Convert to 16-bit unsigned integers and return combined projection.
	return (clampedScreenYTop << 16) | clampedScreenYLow;
}


