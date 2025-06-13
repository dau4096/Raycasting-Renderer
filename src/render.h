#ifndef RENDER_H
#define RENDER_H

#include "includes.h"
#include "global.h"
#include "utils.h"
#include <array>



namespace render {
	GLFWwindow* initializeWindow(int width, int height, const char* title);
	GLuint createShaderProgram(std::string name, bool hasVertexSource=true);


	
	GLuint createShaderStorageBufferObject(int binding, size_t bufferSize=0) {
		GLuint SSBO;
		glGenBuffers(1, &SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return SSBO;
	}

	template<typename TGPU, typename TCPU>
	void updateShaderStorageBufferObject(
			GLuint SSBO,
			std::vector<TCPU>* dataSetIn,
			std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>* symbolNames //Only used for TOs
		) {

		size_t singleItemSize = sizeof(TGPU);
		size_t size = dataSetIn->size();
		std::vector<TGPU> dataSet;

		for (size_t index=0; index<size; index++) {
			dataSet.push_back(TGPU(dataSetIn->data() + index, symbolNames));
		}

		if (size > 0 && !dataSet.empty()) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, singleItemSize * size, dataSet.data());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}

	template<typename TGPU, typename TCPU>
	void updateShaderStorageBufferObject(
			GLuint SSBO,
			std::vector<TCPU>* dataSetIn
		) {

		size_t singleItemSize = sizeof(TGPU);
		size_t size = dataSetIn->size();
		std::vector<TGPU> dataSet;

		for (size_t index=0; index<size; index++) {
			dataSet.push_back(TGPU(dataSetIn->data() + index));
		}

		if (size > 0 && !dataSet.empty()) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, singleItemSize * size, dataSet.data());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
	}



	void saveScreenshot(GLuint frameTextureID);

	GLuint createGLImage2D(int width, int height);
	GLuint loadGLTexture2D(const std::string textureName, std::string subFolder="textures-env", int expectedWidth=-1, int expectedHeight=-1);
	GLuint createTexture2DArray(std::array<std::string, display::TEXTURE_ARRAY_MAX_LAYERS>& textureNames, std::string subFolder="textures-env");


	GLuint getVAO();

	float viewBob(float tick, utils::Player player);
	glm::vec4 manageScreenTint(int newDuration=0, unsigned int event=E_NONE);
}

#endif
