#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include "GLHelper.h"

//Convertes 3D to 1D indices
void GLHelper::index_3D_to_1D(int x, int y, int z, int xSize, int ySize, int* index)
{
	*index = x + xSize * y + xSize * ySize * z;
}

//Convertes 1D to 3D indices
void GLHelper::index_1D_to_3D(int index, int xSize, int ySize, int zSize, int* x, int* y, int* z)
{
	*z = index / (xSize * ySize);
	index %= (xSize * ySize);
	*y = index / xSize;
	*x = index % xSize;
}

//Debugs linking errors
void GLHelper::debug_linking(unsigned int shaderProgram, unsigned int vertexShader, unsigned int fragmentShader)
{
    int success;
    char infoLog[512];

    // Vertex shader
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex Shader compilation failed:\n" << infoLog << std::endl;
    }

    // Fragment shader
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment Shader compilation failed:\n" << infoLog << std::endl;
    }

    // Program linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader Program linking failed:\n" << infoLog << std::endl;
    }
}

//Rotates the camera based on mouse delta
void GLHelper::rotate_camera(glm::vec3* camRot, float* lastX, float* lastY, float xPos, float yPos, float sensitivity)
{
    float deltaX = xPos - *lastX;
    float deltaY = yPos - *lastY;
    *lastX = xPos;
    *lastY = yPos;
    camRot->x -= deltaY * sensitivity;
    if (camRot->x > 89.0f)
        camRot->x = 89.0f;
    if (camRot->x < -89.0f)
        camRot->x = -89.0f;
    camRot->y -= deltaX * sensitivity;
}

//Creates a rotation matrix
glm::mat4 GLHelper::calculate_rotation_matrix(glm::vec3 rotation)
{
    glm::mat4 r = glm::mat4(1.0f);
    r = glm::rotate(r, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    r = glm::rotate(r, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    r = glm::rotate(r, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    return r;
}

//Calculates a matrix to go from world to view space
glm::mat4 GLHelper::calculate_viewspace_matrix(glm::vec3 translation, glm::vec3 rotation)
{
    glm::mat4 t = glm::mat4(1.0f);
    t = glm::translate(t, -translation);
    glm::mat4 r = GLHelper::calculate_rotation_matrix(-rotation);
    return r * t;
}

//Moves a position vector based on the pitch and yaw of its rotation
void GLHelper::move_with_pitch(GLFWwindow* window, float moveSpeed, glm::vec3* camPos, glm::vec3 camRot)
{
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        direction.y += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        direction.y -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        direction.x -= 1;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        direction.x += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        direction.z += 1;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        direction.z -= 1;
    }

    if (direction != glm::vec3(0.0f, 0.0f, 0.0f))
        direction = glm::normalize(direction);
    float pitch = glm::radians(camRot.x);
    float yaw = glm::radians(camRot.y + 90.0f);

    glm::vec3 forward;
    forward.x = cos(yaw) * cos(pitch);
    forward.y = sin(pitch);
    forward.z = -sin(yaw) * cos(pitch);
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    *camPos += (direction.y * forward + direction.x * right + direction.z) * moveSpeed;
}

//Reads shader code into a string
std::string GLHelper::read_shader(std::string fileName)
{
    std::string line;
    std::string source;

    std::ifstream file(fileName);
    while (getline(file, line))
    {
        source += line;
        source += "\n";
    }

    file.close();
    return source;
}

//Generates a shader program and binds shaders
void GLHelper::gen_shader_program(const char* vertexShaderSource, const char* fragmentShaderSource, unsigned int* program)
{
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    debug_linking(shaderProgram, vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    *program = shaderProgram;
}

// Hash for float 3D coordinates (deterministic, continuous-friendly)
float GLHelper::hash_3D(const glm::vec3& p)
{
    // Large primes for mixing
    const glm::vec3 prime = glm::vec3(127.1f, 311.7f, 74.7f);
    float h = glm::dot(p, prime);
    // Fractional part of sine mix
    float hash = (sin(h) * 43758.5453123f);
    return hash - floor(hash);
}

//Generate Worley noise at a single point
float GLHelper::worley_3D(glm::vec3 p)
{
    glm::ivec3 cell = glm::floor(p);
    glm::vec3 localPos = glm::fract(p);
    float minDist = 1e9f;

    for (int xo = -1; xo <= 1; xo++) {
        for (int yo = -1; yo <= 1; yo++) {
            for (int zo = -1; zo <= 1; zo++) {
                glm::ivec3 neighbor = cell + glm::ivec3(xo, yo, zo);
                glm::vec3 pointOffset(
                    hash_3D(glm::vec3(neighbor.x + 11, neighbor.y, neighbor.z)),
                    hash_3D(glm::vec3(neighbor.x, neighbor.y + 17, neighbor.z)),
                    hash_3D(glm::vec3(neighbor.x, neighbor.y, neighbor.z + 23))
                );
                glm::vec3 diff = glm::vec3(xo, yo, zo) + pointOffset - localPos;
                float dist = glm::dot(diff, diff);
                if (dist < minDist)
                    minDist = dist;
            }
        }
    }

    return std::sqrt(minDist);
}

//Generate fractal worley noise
float GLHelper::fractal_worley(glm::vec3 p)
{
    float noise = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float totalAmplitude = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        noise += worley_3D(p * frequency) * amplitude;
        totalAmplitude += amplitude;
        amplitude *= 0.5;      // reduce weight each octave
        frequency *= 2.0;      // increase frequency each octave
    }
    return noise / totalAmplitude;
}


void GLHelper::gen_render_buffers(unsigned int* VAO, unsigned int* VBO, unsigned int* EBO, float vertices[], float sizeOfVertices, unsigned int indices[],float sizeOfIndices)
{
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glGenBuffers(1, EBO);

    glBindVertexArray(*VAO);

    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeOfVertices, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeOfIndices, indices, GL_STATIC_DRAW);
}

void GLHelper::create_framebuffer(unsigned int* FBO, unsigned int* framebufferTexture, unsigned int* RBO, int width, int height)
{
    glGenFramebuffers(1, FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

    // Create color texture
    glGenTextures(1, framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, *framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); // use float for higher precision
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *framebufferTexture, 0);

    // Create depth + stencil renderbuffer
    glGenRenderbuffers(1, RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, *RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *RBO);

    // Validate
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR: Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLHelper::resize_framebuffer(unsigned int framebufferTexture, unsigned int RBO, int width, int height)
{
    // Reallocate color texture
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

    // Reallocate renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
