#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class GLHelper
{
public:
    static void index_3D_to_1D(int x, int y, int z, int xSize, int ySize, int* index);
    static void index_1D_to_3D(int index, int xSize, int ySize, int zSize, int* x, int* y, int* z);
    static void debug_linking(unsigned int shaderProgram, unsigned int vertexShader, unsigned int fragmentShader);
    static void rotate_camera(glm::vec3* camRot, float* lastX, float* lastY, float xPos, float yPos, float sensitivity);
    static glm::mat4 calculate_rotation_matrix(glm::vec3 rotation);
    static glm::mat4 calculate_viewspace_matrix(glm::vec3 translation, glm::vec3 rotation);
    static std::string read_shader(std::string fileName);
    static void gen_shader_program(const char* vertexShaderSource, const char* fragmentShaderSource, unsigned int* program);
    static void move_with_pitch(GLFWwindow* window, float moveSpeed, glm::vec3* camPos, glm::vec3 camRot);
    static float hash_3D(const glm::vec3& p);
    static float worley_3D(glm::vec3 p);
    static float fractal_worley(glm::vec3 p);
    static void gen_render_buffers(unsigned int* VAO, unsigned int* VBO, unsigned int* EBO, float vertices[], float sizeOfVertices, unsigned int indices[], float sizeOfIndices);
    static void create_framebuffer(unsigned int* FBO, unsigned int* framebufferTexture, unsigned int* RBO, int width, int height);
    static void resize_framebuffer(unsigned int framebufferTexture, unsigned int RBO, int width, int height);
};