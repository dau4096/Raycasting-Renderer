#ifndef RENDER_H
#define RENDER_H

#include "includes.h"
#include "utils.h"
#include <array>  // Include this header for std::array

namespace render {
    GLFWwindow* initializeWindow(int width, int height, const char* title);
    GLuint createShaderProgram(std::string name, bool hasVertexSource);
    void createConstUBO();
    void createWallUBO(const std::array<utils::Wall, 256>* dataSet);
    GLuint createSpriteUBO();
    void updateSpriteUBO(GLuint* spriteUBO, const std::array<utils::Sprite, 32>* dataSet);
    void createLightUBO(const std::array<utils::Light, 32>* dataSet);
    GLuint createTexture(int width, int height);
    GLuint createTextureArray(const std::array<std::string, 32>& textureNames);
    GLuint getVAO();
}

#endif
