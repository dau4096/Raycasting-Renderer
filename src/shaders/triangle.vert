#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aUV;
layout(location=2) in float aType;

out vec3 fragUV;
out vec3 fragPos;
out flat int fragType;

uniform mat4 perspectiveMatrix;
uniform mat4 orthoMatrix;

void main() {
    fragPos = aPos;
    fragUV = aUV;
    fragType = int(floor(aType + 0.5f));

    vec4 perspPos = perspectiveMatrix * vec4(aPos, 1.0);
    vec4 orthoPos = orthoMatrix * vec4(aPos, 1.0);

    gl_Position = vec4(perspPos.x, perspPos.y, orthoPos.z, perspPos.w);
}
