#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "GLHelper.h"

glm::vec3 camRotation;
glm::vec3 camPosition;
float lastTime;
float deltaTime;
float lastX;
float lastY;
bool uWasPressed;
bool showUI = true;
unsigned int framebufferTexture;
unsigned int RBO;


int screenWidth = 800;
int screenHeight = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{ 
    GLHelper::resize_framebuffer(framebufferTexture, RBO, width, height);
    glViewport(0, 0, width, height);
    screenWidth = width;
    screenHeight = height;
}

void process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        if (!uWasPressed)
        {
            uWasPressed = true;
            showUI = !showUI;
        }
    }
    else
    {
        uWasPressed = false;
    }
}

unsigned int create_noise_texture(glm::ivec3 s, int resolution, float noiseScalePos, float noiseScaleNeg, float densityFalloff, float densityScale)
{
    glm::ivec3 size = s * resolution;
    int length = size.x * size.y * size.z;
    std::vector<float> noise(length );

    for (int i = 0; i < length; i++)
    {
        int xi, yi, zi;
        GLHelper::index_1D_to_3D(i, size.x, size.y, size.z, &xi, &yi, &zi);
        float x = static_cast<float>(xi) / resolution;
        float y = static_cast<float>(yi) / resolution;
        float z = static_cast<float>(zi) / resolution;
        (noise[i] = (GLHelper::fractal_worley(glm::vec3(x, y, z) * noiseScalePos)- 0.5f) * 5.0f - GLHelper::fractal_worley(glm::vec3(x, y, z) * noiseScaleNeg) -(exp(abs(0.3f - y / size.y) * densityFalloff)-1.0f)) * densityScale;

    }
    unsigned int noiseTex;
    glGenTextures(1, &noiseTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, noiseTex);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, size.x, size.y, size.z, 0, GL_RED, GL_FLOAT, noise.data());

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return noiseTex;
}

float vertices[] = {
    1.0f,  1.0f, 0.0f,  
     1.0f, -1.0f, 0.0f, 
    -1.0f,  1.0f, 0.0f, 

     1.0f, -1.0f, 0.0f,  
    -1.0f, -1.0f, 0.0f,  
    -1.0f,  1.0f, 0.0f
};

unsigned int indices[] = {
    0, 1, 2,
    3, 4, 5
};

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RED_BITS, 16);
    glfwWindowHint(GLFW_GREEN_BITS, 16);
    glfwWindowHint(GLFW_BLUE_BITS, 16);
    glfwWindowHint(GLFW_ALPHA_BITS, 16);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Volume Raymarching", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io; // silence unused warning

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    

    unsigned int VAO, VBO, EBO;

    GLHelper::gen_render_buffers(&VAO, &VBO, &EBO, vertices, sizeof(vertices), indices, sizeof(indices));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int rayMarchShaderProgram;
    rayMarchShaderProgram = glCreateProgram();

    std::string rayMarchVertexShaderCode = GLHelper::read_shader("RayMarchVertex.shader");
    std::string rayMarchFragmentShaderCode = GLHelper::read_shader("RayMarchFragment.shader");

    const char* rayMarchVertexShaderSource = rayMarchVertexShaderCode.c_str();
    const char* rayMarchFragmentShaderSource = rayMarchFragmentShaderCode.c_str();

    GLHelper::gen_shader_program(rayMarchVertexShaderSource, rayMarchFragmentShaderSource, &rayMarchShaderProgram);

    unsigned int displayShaderProgram;

    std::string displayVertexShaderCode = GLHelper::read_shader("DisplayVertex.shader");
    std::string displayFragmentShaderCode = GLHelper::read_shader("DisplayFragment.shader");

    const char* displayVertexShaderSource = displayVertexShaderCode.c_str();
    const char* displayFragmentShaderSource = displayFragmentShaderCode.c_str();

    GLHelper::gen_shader_program(displayVertexShaderSource, displayFragmentShaderSource, &displayShaderProgram);

    unsigned int FBO;
    GLHelper::create_framebuffer(&FBO, &framebufferTexture, &RBO, screenWidth, screenHeight);

    glUseProgram(rayMarchShaderProgram);

    glUniform1f(glGetUniformLocation(rayMarchShaderProgram, "nearPlane"), 0.1f);
    glUniform1f(glGetUniformLocation(rayMarchShaderProgram, "farPlane"), 1000.0f);
    glm::vec3 boxMin = glm::vec3(0.0f,1.0f,0.0f);
    glm::vec3 boxMax = glm::vec3(4.0f,5.0f,4.0f);
    glUniform3f(glGetUniformLocation(rayMarchShaderProgram, "volumeBoxMin"), boxMin.x, boxMin.y, boxMin.z);
    glUniform3f(glGetUniformLocation(rayMarchShaderProgram, "volumeBoxMax"), boxMax.x, boxMax.y, boxMax.z);
    glUniform3f(glGetUniformLocation(rayMarchShaderProgram, "sunColor"), 1.0f, 1.0f,1.0f);
    glUniform3f(glGetUniformLocation(rayMarchShaderProgram, "sunPos"), 5.0f, 10.0f, 0.0f);
    glm::vec3 boxSizeCeil = glm::ceil(boxMax - boxMin);

    int viewRayNumSteps = 50;
    int shadowRayNumSteps = 50;
    int textureResolution = 16;
    float absorption = 0.5f;
    float noiseScalePos = 0.125f;
    float noiseScaleNeg = 5.0f;
    float phaseLobeStrength = 0.85f;
    float densityScale = 1.0f;
    float densityFalloff = 0.5f;

    unsigned int noiseTex = create_noise_texture(glm::ivec3(
        static_cast<int>(boxSizeCeil.x), static_cast<int>(boxSizeCeil.y),
        static_cast<int>(boxSizeCeil.z)
    ), textureResolution ,noiseScalePos, noiseScaleNeg,densityFalloff,densityScale);

    glUniform1i(glGetUniformLocation(rayMarchShaderProgram, "volume"), 0);

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        ImGui_ImplGlfw_CursorPosCallback(w, x, y); // forward to ImGui
        ImGuiIO& io = ImGui::GetIO();
        if (!showUI) {
            GLHelper::rotate_camera(&camRotation, &lastX, &lastY, x, y, 0.02f);
        }
        });

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glViewport(0, 0, 800, 600);
    glClearColor(1.0f, 0.0f, 0.5f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        if (showUI)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Example ImGui window
            ImGui::Begin("Raymarch Settings");
            ImGui::Text("Press 'u' to toggle UI");
            ImGui::SliderInt("View Ray Steps", &viewRayNumSteps, 1, 256);
            ImGui::SliderInt("Shadow Ray Steps", &shadowRayNumSteps, 1, 256);
            ImGui::SliderFloat("Absorption", &absorption, 0.0f, 1.0f);
            ImGui::SliderFloat("Phase Lobe Strength", &phaseLobeStrength, -1.0f, 1.0f);
            ImGui::SliderFloat("Density Scale", &densityScale, 0.0f, 10.0f);
            ImGui::SliderFloat("Density Falloff", &densityFalloff, 0.0f, 1.0f);
            ImGui::SliderFloat("Main Noise Scale", &noiseScalePos, 0.001f, 100.0f);
            ImGui::SliderFloat("Detail Noise Scale", &noiseScaleNeg, 0.01f, 100.0f);
            ImGui::SliderInt("Texture Resolution", &textureResolution, 1, 128);
            if (ImGui::Button("Regenerate Texture"))
            {
                glDeleteTextures(1, &noiseTex);
                noiseTex = create_noise_texture(glm::ivec3(
                    static_cast<int>(boxSizeCeil.x), static_cast<int>(boxSizeCeil.y),
                    static_cast<int>(boxSizeCeil.z)
                ), textureResolution, noiseScalePos, noiseScaleNeg, densityFalloff, densityScale);
            }
            ImGui::End();

            // --- Rendering ---
            ImGui::Render();
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }


        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float time = glfwGetTime();
        deltaTime = time - lastTime;
        lastTime = time;

        glUseProgram(rayMarchShaderProgram);

        glUniform1f(glGetUniformLocation(rayMarchShaderProgram, "absorption"), absorption);
        glUniform1f(glGetUniformLocation(rayMarchShaderProgram, "phaseLobeStrength"), phaseLobeStrength);
        glUniform1i(glGetUniformLocation(rayMarchShaderProgram, "viewRayNumSteps"), viewRayNumSteps);
        glUniform1i(glGetUniformLocation(rayMarchShaderProgram, "shadowRayNumSteps"), shadowRayNumSteps);

        GLHelper::move_with_pitch(window, 3.0f * deltaTime, &camPosition, camRotation);

        glm::mat4 view = GLHelper::calculate_viewspace_matrix(camPosition, glm::radians(camRotation));
        glUniformMatrix4fv(glGetUniformLocation(rayMarchShaderProgram, "viewMatrix"),1,GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(rayMarchShaderProgram, "invViewMatrix"),1,GL_FALSE, glm::value_ptr(glm::inverse(view)));

        glUniformMatrix4fv(glGetUniformLocation(rayMarchShaderProgram, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
        glUniformMatrix4fv(glGetUniformLocation(rayMarchShaderProgram, "invModelMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));

        glUniform2f(glGetUniformLocation(rayMarchShaderProgram, "resolution"), screenWidth, screenHeight);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(displayShaderProgram);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1i(glGetUniformLocation(displayShaderProgram, "screenTexture"), 0);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        if (showUI)
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        process_input(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // --- Cleanup ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}