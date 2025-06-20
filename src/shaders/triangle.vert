/* triangle.vert */
#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aUV;
layout(location=2) in float aType;

out vec3 fragUV;
out vec3 fragPos;
out flat int fragType;
uniform mat4 pvmMatrix;

void main() {
    fragPos = aPos;
    fragUV = aUV;
    fragType = int(floor(aType + 0.5f));

    gl_Position = pvmMatrix * vec4(aPos, 1.0);
}