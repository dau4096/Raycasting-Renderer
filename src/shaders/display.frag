/* display.frag */
#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;

layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
    float zoomFactor;
    float maxRayAngle;
    float maxRayDistance;
    float dimmingStrength;

    vec3 topColour;
    vec3 lowColour;

    float padding[3];
};


void main() {
    fragColour = vec4(imageLoad(renderedFrame, ivec2(gl_FragCoord.xy)).rgb, 1.0f);
}