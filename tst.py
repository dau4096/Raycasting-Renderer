

def unpackIndices(packed:int) -> tuple[bool,int]:
	index:int = packed >> 1;
	return (
		(packed & 0x1) == 1,
		index
	);


def unpackProjections(packed:int) -> tuple[int,int]:
	"""
	uint biasedHigher = uint(higher + 0x7FFF) & 0xFFFF;
	uint biasedLower = uint(lower + 0x7FFF) & 0xFFFF;

	return (biasedHigher << 16) | biasedLower;
	"""
	biasedHigher:int = (packed >> 16) & 0xFFFF;
	biasedLower:int  = (packed >>  0) & 0xFFFF;

	higher:int = biasedHigher - 0x7FFF;
	lower:int  = biasedLower  - 0x7FFF;

	return (lower, higher);


def getValidityProjections(uvec2:tuple[int,int]) -> bool:
	verdict:bool = True;

	(intersectionFound, index) = unpackIndices(uvec2[0]);
	(lower, higher) = unpackProjections(uvec2[1]);

	if (intersectionFound):
		print(f"Found IDX: {index}, LOWER: {lower}, HIGHER: {higher}");
	else:
		print(f"Not found.");

	if (not intersectionFound):
		print("Intersection-bit was not true.");
		verdict = False;
	if ((higher == 32768) and (lower == -32767)):
		print("Range was marked as invalid.");
		verdict = False;
	if ((lower > 1920) and (higher > lower)):
		print("Range was above screen Y range.");
		verdict = False;
	if ((higher < 0) and (lower < higher)):
		print("Range was below screen Y range.");
		verdict = False;
	if (index >= 34):
		print("Index was outside of range of valid visplane indices.");
		verdict = False;

	return verdict;


UV_SCALE_CONSTANT:int = 65535.0;
def unpackInvUV(en):
	return float(en) / UV_SCALE_CONSTANT;

def unpackWallTextureUVs(en):
	return (unpackInvUV(en >> 16), unpackInvUV(en & 0xFFFF));

def getValidityWallUV(uint:int) -> bool:
	hUVy, lUVy = unpackWallTextureUVs(uint);
	print(f"HigherUV: {hUVy}, LowerUV: {lUVy}");
	return True;

def getNumRepeats(uint:int) -> int:
	print(hex(uint)[2:].zfill(8), end=" ");
	return (uint >> 8) & 0xFF;



#Visplane Projections
#getValidityProjections((5, 2156920832));
#getValidityProjections((5, 2425852055));


#Wall things
getValidityWallUV(3212836864);
getValidityWallUV(1065353216);
getValidityWallUV(1090519040);
getValidityWallUV(3238002688);
getValidityWallUV(4292935672);
print(getNumRepeats(524918));
