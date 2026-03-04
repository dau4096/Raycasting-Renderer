/* stack.glsl */
//Contains functions related to the intersection "stack".


struct IntersectionData {
	vec3 position;		//3D intersect location
	vec3 UV;			//UV & texture ID
	float distanceSQ;	//Distance from camera, squared
	uint index;			//The index of the found object
	int foundType;		//The type of the found object
	vec2 normal2D;		//Normal vector of the intersect.
	bool isPortal;		//No lighting applied.
};
#define STACK_SIZE 8
IntersectionData stack[STACK_SIZE];
uint topOfStack = 0;

void pushStack(in IntersectionData data) {
	if (topOfStack == 0) {
		stack[0] = data;
		topOfStack = 1;
		return;
	}

	uint insertIdx = topOfStack;
	for (int i=0; i<topOfStack; i++) {
		if (data.distanceSQ < stack[i].distanceSQ) {
			insertIdx = i;
			break;
		}
	}

	if ((topOfStack == STACK_SIZE) && (insertIdx == STACK_SIZE)) {
		return;
	}

	if (topOfStack < STACK_SIZE) {
		topOfStack++;
	}

	for (uint i=topOfStack-1; i>insertIdx; i--) {
		stack[i] = stack[i - 1];
	}

	stack[insertIdx] = data;
}

//Unused;
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