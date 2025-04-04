#ifndef RENDER_H
#define RENDER_H

#include "includes.h"
#include "utils.h"
#include <array>  // Include this header for std::array

namespace render {
    GLFWwindow* initializeWindow(int width, int height, const char* title);
    GLuint createShaderProgram(std::string name, bool hasVertexSource=true);


    void createConstUBO();

    GLuint createVisplaneUBO();
    void updateVisplaneUBO(GLuint visplaneUBO, std::array<utils::Visplane, constants::MAX_VISPLANES>* dataSet);

    GLuint createWallUBO();
    void updateWallUBO(GLuint wallUBO, std::array<utils::Wall, constants::MAX_WALLS>* dataSet);

    GLuint createSpriteSSBO();
    void updateSpriteSSBO(GLuint spriteSSBO, std::array<utils::Sprite, constants::MAX_SPRITES>* dataSet);

    GLuint createLightSSBO();
    void updateLightSSBO(GLuint lightSSBO, std::array<utils::Light, constants::MAX_LIGHTS>* dataSet);

    GLuint createDepthSSBO(int width);


    GLuint createTexture(int width, int height);
    GLuint createTextureArray(const std::array<std::string, 32>& textureNames);


    GLuint getVAO();

    float viewBob(float tick, utils::Player player);
    glm::vec4 manageScreenTint(glm::uint newDuration=0, glm::uint event=utils::E_NONE);
}

#endif
