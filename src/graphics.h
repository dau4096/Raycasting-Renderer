#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "includes.h"
#include "global.h"
#include "utils.h"
#include <array>

using namespace glm;


namespace graphics {
	GLFWwindow* initialiseWindow(int width, int height, const char* title);
	static inline GLuint createShaderProgram(std::string fragShaderName, bool hasVertexSource=true);
	static inline GLuint createShaderProgram(std::string fragShaderName, std::string vertexShaderName);
	GLuint createComputeShader(std::string compShaderName);


	
	GLuint createShaderStorageBufferObject(int binding, size_t bufferSize=0, GLuint glType=GL_DYNAMIC_DRAW);

	template<typename TGPU, typename TCPU>
	void updateShaderStorageBufferObject(GLuint SSBO, utils::Player* player, std::vector<TCPU>* dataSetIn);

	template<typename TGPU, typename TCPU>
	void updateShaderStorageBufferObject(GLuint SSBO, std::vector<TCPU>* dataSetIn);

	template<typename T>
	void updateShaderStorageBufferObject(GLuint SSBO, T* data, size_t count);




	void findVisibleObjects(
		utils::Player* player,
		std::vector<utils::Visplane>* visplaneData, std::vector<uint>* visibleVisplaneIndices,
		std::vector<utils::Wall>* wallData, std::vector<uint>* visibleWallIndices,
		std::vector<utils::Displacement>* displacementData, std::vector<uint>* visibleDisplacementsIndices
	);

	void saveScreenshot(GLuint frameTextureID);

	GLuint createGLImage2D(size_t width, size_t height, GLint internalFormat=GL_RGBA32F, GLint samplingType=GL_NEAREST, GLint edgeSampling=GL_REPEAT);
	GLuint loadGLTexture2D(const std::string textureName, std::string subFolder="textures-env", int expectedWidth=-1, int expectedHeight=-1);
	GLuint createTexture2DArray(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>& textureNames, std::string subFolder="textures-env", bool hasMipMap=false);
	GLuint createGLImage2DArray(size_t width, size_t height, size_t layers);


	GLuint getVAO();

	float viewBob(float tick, utils::Player player);
	glm::vec4 manageScreenTint(utils::Player* player);


	void initialiseVAOs();
	void prepareOpenGL(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* textureNames, utils::Player* player);

	GLuint createDisplacementsFBO(size_t width, size_t height);

}



namespace frame {

	void updateSSBOs(utils::DataSet* localGraphicsData, utils::Player* player);

	void draw(double blendingAlpha, utils::DataSet* localGraphicsData, utils::Player* player);

}

#endif
