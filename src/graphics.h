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
	void updateShaderStorageBufferObject(GLuint SSBO, structs::Player* player, std::vector<TCPU>* dataSetIn);

	template<typename TGPU, typename TCPU>
	void updateShaderStorageBufferObject(GLuint SSBO, std::vector<TCPU>* dataSetIn);

	template<typename T>
	void updateShaderStorageBufferObject(GLuint SSBO, T* data, size_t count);




	void findVisibleObjects(
		structs::Player* player,
		std::vector<structs::Visplane>* visplaneData, std::vector<uint>* visibleVisplaneIndices,
		std::vector<structs::Wall>* wallData, std::vector<uint>* visibleWallIndices,
		std::vector<structs::Displacement>* displacementData, std::vector<uint>* visibleDisplacementsIndices
	);

	void saveScreenshot(GLuint frameTextureID);


	//Textures
	GLuint createGLImage2D(size_t width, size_t height, GLint internalFormat=GL_RGBA32F, GLint samplingType=GL_NEAREST, GLint edgeSampling=GL_REPEAT);
	GLuint loadGLTexture2D(const std::string textureName, std::string subFolder="textures-env", int expectedWidth=-1, int expectedHeight=-1);
	GLuint createTexture2DArray(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>& textureNames, std::string subFolder="textures-env", bool hasMipMap=false);
	void writeToSpecificTexture2DArrayLayer(GLuint sheetArrayID, std::string textureName, size_t layer, bool hasMipMap=false);
	GLuint createGLImage2DArray(size_t width, size_t height, size_t layers, GLenum filtering=GL_NEAREST);
	GLint fetchTextureID(std::string textureName, std::string subFolder="textures-env");
	void handleTextureLoadQueue();


	GLuint getVAO();

	float viewBob(float tick);
	glm::vec4 manageScreenTint();


	GLuint createAlbedoFBO(glm::uvec2 resolution, GLuint& colourTexture);
	GLuint createDualAlbedoFBO(glm::uvec2 resolution, GLuint& colourTextureA, GLuint& colourTextureB);
	GLuint createAlbedoDepthFBO(glm::uvec2 resolution, GLuint& colourTexture, GLuint& depthTexture);
	GLuint createEnvironmentFBO(glm::uvec2 resolution);


	void initialiseVAOs();
	void prepareOpenGL();

	GLuint createDisplacementsFBO(size_t width, size_t height);


}



namespace lighting {

	void createLightMapsAll();

	void createLightMapsSubset(std::vector<unsigned int>& visplanes, std::vector<unsigned int>& walls);

}



namespace frame {

	void updateSSBOs(bool drawLightBlockers);
	void draw(double blendingAlpha, double currentTime);

}



namespace particles {

void createParticle(glm::vec3 position, ParticleType type=P_DUST, glm::vec3 initialVelocity=glm::vec3(0.0f, 0.0f, 0.0f));
void createParticleLine(glm::vec3 start, glm::vec3 end, ParticleType type=P_DUST, size_t numParticles=0);
void createParticleRing(glm::vec3 centre, float radius, ParticleType type=P_DUST, size_t numParticles=0);

}

#endif
