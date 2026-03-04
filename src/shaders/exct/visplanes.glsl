/* visplanes.glsl */
//Contains all visplane visual processing funcs.


vec2 getVertex(Visplane plane, uint index) {
	uint actualIndex = index / 2;
	return (index % 2 == 0) ? plane.vertices[actualIndex].xy : plane.vertices[actualIndex].zw;
}

bool isInsideVP(vec2 point2D, Visplane thisVisplane) {
	for (uint i=0; i<thisVisplane.numVertices; i++) {
		vec2 a = getVertex(thisVisplane, i);
		vec2 b = getVertex(thisVisplane, (i + 1) % thisVisplane.numVertices);
		vec2 edge = b - a;
		vec2 toPoint = point2D - a;
		vec2 normal = vec2(-edge.y, edge.x); //90° Anti-Clockwise

		if (dot(normal, toPoint) < EPSILON_ALT) {return false; /* Point is outside the edge */}
	}
	return true;
}


vec2 getVisplaneUV(vec2 position2D, Visplane plane, out int textureID, out bvec4 textureFlags) {
	//Unpack formatting bits
	unpackTextureFormattingBits1(plane.textureData1, textureFlags, textureID);
	bvec2 useWorldSpace;
	vec2 invTextureScale, textureOffset;
	unpackTextureFormattingBits2(plane.textureData2, useWorldSpace, invTextureScale, textureOffset);

	//Select UV type from bits;
	vec2 uv = mix(
		(position2D - plane.boundingBox.xy) / (plane.boundingBox.xy - plane.boundingBox.zw),	//Local UV
		position2D * invTextureScale,															//World UV
		useWorldSpace
	);

	return uv + textureOffset;
}

