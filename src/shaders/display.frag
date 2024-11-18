#version 460 core

in vec2 fragTexCoord;
out vec4 fragColour;

uniform sampler2D frameBufferID;

void main() {
    fragColour = texture(frameBufferID, fragTexCoord);
    
    //fragColour = vec4(1.0, 0.0, 1.0, 1.0);
}