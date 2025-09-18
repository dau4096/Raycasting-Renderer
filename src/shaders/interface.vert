/* interface.vert */
#version 460 core

layout(location=0) in vec2 aPos;
layout(location=1) in vec3 aUV;
layout(location=2) in float aDistance;


out vec3 fragUV;
out vec3 fragPos;
out flat float fragDistance;


uniform mat4 pvmMatrix;

void main() {
    int intZ = int(floor(aUV.z + 0.5f));
    float Z = ((intZ & 2) > 0) ? -0.5f : 0.0f;
    fragPos = vec3(aPos, Z);
    fragUV = aUV;
    fragDistance = aDistance;

    gl_Position = pvmMatrix * vec4(aPos.xy, Z, 1.0);
}