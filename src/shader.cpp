void useShader(GLuint shader, GLenum permission, GLuint frameTextureID, GLuint VAO) {
	glUseProgram(shader);
	glBindImageTexture(0, frameTextureID, 0, GL_FALSE, 0, permission, GL_RGBA32F);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	utils::GLErrorcheck("ShaderCall", true);
}