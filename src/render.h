#ifndef RENDER_H
#define RENDER_H

#include "includes.h"
#include "utils.h"
#include <array>

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


    GLuint createGLImage2D(int width, int height);
    GLuint loadGLTexture2D(const std::string textureName, int expectedWidth=-1, int expectedHeight=-1);
    GLuint createTexture2DArray(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>& textureNames);


    GLuint getVAO();

    float viewBob(float tick, utils::Player player);
    glm::vec4 manageScreenTint(int newDuration=0, unsigned int event=E_NONE);
}

#endif
