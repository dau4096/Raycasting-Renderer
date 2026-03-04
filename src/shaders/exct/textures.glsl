/* textures.glsl */
//Texture-specific processing funcs


void unpackTextureFormattingBits1(
		uint inputBits, out bvec4 textureFlags,
		out int textureID
	) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	- textureFlags; (4 bit flags) [0 - 15]
		1111 0000 0000 0000 0000 0000 0000 0000
	- textureID; (12 bit uint) [0 - 65535]
		0000 1111 1111 1111 0000 0000 0000 0000
	*/
	textureFlags = bvec4(
		bool(inputBits & 0x80000000),
		bool(inputBits & 0x40000000),
		bool(inputBits & 0x20000000),
		bool(inputBits & 0x10000000)
	);

	uint texIDbits = (inputBits >> 16) & 0xFFF;
	textureID = (texIDbits == 0xFFF) ? -1 : int(texIDbits);
}
void unpackTextureFormattingBits2(
		uint inputBits, out bvec2 isWorldspace,
		out vec2 invTextureScale, out vec2 textureOffset
	) {
	/*
	- Full 32bits; (uint)
		0000 0000 0000 0000 0000 0000 0000 0000
	isWorldspace.x; (bool) [0 / 1]
		1000 0000 0000 0000 0000 0000 0000 0000
	- isWorldspace.y; (bool) [0 / 1]
		0100 0000 0000 0000 0000 0000 0000 0000
	- textureScale.x; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0011 1111 1100 0000 0000 0000 0000 0000
	- textureScale.y; ((8-bit uint) / 16.0f) [0.0 - 16.0]
		0000 0000 0011 1111 1100 0000 0000 0000
	- textureOffset.x; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0011 1111 1000 0000
	- textureOffset.y; ((7-bit uint) / 128.0f) [0.0 - 1.0]
		0000 0000 0000 0000 0000 0000 0111 1111
	*/

	isWorldspace = bvec2(
		bool(inputBits & 0x80000000),
		bool(inputBits & 0x40000000)
	);

	invTextureScale = 16.0f / vec2(
		float((inputBits >> 22) & 0xFF),
		float((inputBits >> 14) & 0xFF)
	);

	textureOffset = vec2(
		float((inputBits >> 7) & 0x7F),
		float((inputBits >> 0) & 0x7F)
	) / 128.0f;
}



#ifdef TEXTURE_FETCH //Only allow if explicitly told to. Only required sometimes.
void getNormal(vec3 UV, float LODIndex, inout vec3 surfaceNormal, uint surfaceType) {
	vec3 normalMapValue = textureLod(normalMapArray, UV, LODIndex).xyz;
	switch (surfaceType) {
		case T_WALL: {
			//Tangent-space normals.
			vec3 tangentNormal = normalize(normalMapValue * 2.0f - 1.0f);
			vec3 N = surfaceNormal;
			vec3 T = (abs(N.z) > 0.999f) ? vec3(1.0f, 0.0f, 0.0f) : normalize(cross(NORMAL_UP, N));
			vec3 B = cross(N, T);
		
			//Tangent-space to worldspace.
			surfaceNormal = normalize(mat3(T, B, N) * tangentNormal) * vec3(1.0f, 1.0f, -1.0f);
			break;
		}
		case T_VISPLANE: {
			vec3 thisNormal = normalMapValue * 2.0f - 1.0f;
			surfaceNormal = thisNormal * vec3(1.0f, 1.0f, sign(surfaceNormal.z));
			break;
		}

		default: {
			return;
		}
	}
}


vec4 fetchUV(vec3 UV, double distance, inout vec3 surfaceNormal, vec3 surfacePosition, uint surfaceType) {
	if (debugMode == 1) {
		return vec4(abs(fract(UV.xy)), UV.z / 32.0f, 1.0f);
	}

	if (!useMipMapping) {
		return textureLod(textureArray, UV, 0.0);
	}

	if (MIPMAP_FORCE_LEVEL_ENABLED) {
		return textureLod(textureArray, UV, MIPMAP_FORCE_LEVEL_VALUE);
	}

	//LODIndex takes depth and slope components to be as unobtrusive as possible.
	float depthComponent = (MIPMAP_LEVELS * 2.0f / maxRayDistance) * (float(distance) - MIPMAP_MIN_DISTANCE);
	float slopeComponent = -abs(dot(normalize(playerPosition - surfacePosition), surfaceNormal));
	float LODIndex = clamp(depthComponent + slopeComponent, 0.0f, MIPMAP_LEVELS);
	float lod = ceil(LODIndex);


	if (UV.z < 0) {
		//Portal
		return textureLod(portalTexture, fract(UV.xy), lod);
	}

	if (MIPMAP_DEBUG) {
		return vec4(LODIndex / MIPMAP_LEVELS, fract(LODIndex), 0.0f, 1.0f);
	}

	vec4 mipColour = textureLod(textureArray, UV, lod);
	getNormal(UV, lod, surfaceNormal, surfaceType);
	if (debugMode == 2) {
		return vec4(surfaceNormal * 0.5f + 0.5f, 1.0f);
	}
	
	if (!MIPMAP_BLEND_ENABLED) {
		return mipColour;
	}

	vec4 mipColourLow = textureLod(textureArray, UV, lod - 1.0f);
	return mix(mipColourLow, mipColour, fract(LODIndex));
}


vec4 fetchUVIntersect(in IntersectionData thisIntersect, out vec3 surfaceNormal, out bool isPortal) {
	vec3 UV = thisIntersect.UV;
	isPortal = false;
	if (thisIntersect.foundType == T_WALL) {
		surfaceNormal = vec3(thisIntersect.normal2D.xy, 0.0f);
		Wall thisWall = walls[thisIntersect.index];
		if (thisWall.type == 15) { //Portal type.
			vec2 surfaceDirection = vec2(-thisIntersect.normal2D.y, thisIntersect.normal2D.x);
			vec2 fragDirection = normalize(thisIntersect.position.xy - playerPosition.xy);
			float dotProd = 0.5f - abs(dot(fragDirection.xy, surfaceDirection.xy));
			UV = vec3(dotProd, thisIntersect.UV.y, -1.0f);
			isPortal = true;
		}

	} else if (thisIntersect.foundType == T_VISPLANE) {
		Visplane thisVisplane = visplanes[thisIntersect.index];
		surfaceNormal = vec3(0.0f, 0.0f, (thisVisplane.height < playerPosition.z) ? 1.0f : -1.0f);
		if (thisVisplane.type == 15) { //Portal type.
			UV = vec3(fract(thisIntersect.position.xy), -1.0f);
			isPortal = true;
		}

	} else {
		return vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	return fetchUV(
		UV,
		1.0f / inversesqrt(thisIntersect.distanceSQ),
		surfaceNormal,
		thisIntersect.position,
		thisIntersect.foundType
	);
}
#endif

