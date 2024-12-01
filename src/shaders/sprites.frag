/* sprites.frag */
#version 460 core

layout(rgba32f, binding=0) uniform image2D renderedFrame;
layout(std140, binding = 1) uniform constUBO {
    float zoomFactor;
    float maxRayAngle;
    float maxRayDistance;
    float dimmingStrength;

    vec3 topColour;
    vec3 lowColour;

    int drawUV;
};

struct Sprite {
    vec2 position;       // Sprite position
    float width;         // Sprite width
    int textureID;       // Sprite Texture ID
    int valid;           // Sprite; Valid or not?
    float padding[2];    // Padding to align with 16B
};

layout(std140, binding=3) uniform spriteUBO {
    Sprite sprites[32];
};


struct Light {
    vec3 position;      //Light Position
    vec3 colour;        //Light Colour.
    float intensity;    //Light intensity.
    int valid;          //Light; Valid or not?
};
layout(std140, binding=4) uniform lightUBO {
    Light lights[32];
};



void main() {
	//Do nothing, for now.
}