#ifndef RENDER_H
#define RENDER_H

#include "includes.h"
#include "utils.h"
#include <array>  // Include this header for std::array

namespace render {
    GLuint loadShaders();
    GLFWwindow* initializeWindow(int width, int height, const char* title);
    GLuint createTexture();
    void updateTexture(GLuint textureID, utils::FrameBuffer frameBuffer);
    GLuint getVAO();
}

#endif
