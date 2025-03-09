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
    void updateSpriteUBO(GLuint spriteUBO, std::vector<utils::Sprite>* dataSet);
    GLuint createLightUBO();
    void updateLightUBO(GLuint lightUBO, std::vector<utils::Light>* dataSet);
    GLuint createDepthSSBO(int width);
    GLuint createTexture(int width, int height);
    GLuint createTextureArray(const std::array<std::string, 32>& textureNames);
    GLuint getVAO();
}

#endif
