#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader_s.h>
#include <vector>
#include <iostream>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <math.h>
#include "camera.h"
#include"Text.h"
std::vector<float> genBox();
std::vector<float> genRoomBox();
void renderRoomCube();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void renderScene(const Shader& shader);
void renderCube();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));


float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

unsigned int diffuseMapWood;
unsigned int diffuseMapContainer;
unsigned int whiteMap;
unsigned int grassMap;
//light光源
struct Light {
    GLfloat times;
    glm::vec3 position;//光源位置
};
Light light;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    //初始化环境
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //使用深度检测
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Shader shader("shader.vs", "shader.fs");
    Shader simpleDepthShader("shadow.vs", "shadow.fs", "shadow.gs");

    light.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
    light.times = 1.0;
    // 定义深度图FBO
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // 创建深度贴图
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // 将深度贴图绑定到缓冲区
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    diffuseMapWood = loadTexture("wood.png");//加载盒子的贴图
    diffuseMapContainer = loadTexture("container.png");
    whiteMap = loadTexture("white.png");
    grassMap = loadTexture("grass.jpg");

    shader.use();
    shader.setInt("diffuseTexture", 0);
    shader.setInt("depthMap", 1);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        //创建深度立方体贴图变换矩阵
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        //渲染深度贴图
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", light.position);
        renderScene(simpleDepthShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //渲染环境
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        shader.setVec3("lightPos", light.position);
        shader.setFloat("times", light.times);
        shader.setVec3("viewPos", camera.Position);
        shader.setFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMapWood);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        renderScene(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void renderScene(const Shader& shader)
{
    // room cube
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grassMap);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(10.0f));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE);
    shader.setInt("reverse_normals", 1); 
    renderRoomCube();
    shader.setInt("reverse_normals", 0);
    glEnable(GL_CULL_FACE);

    // cubes
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMapWood);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, -8.5f, 0.0));
    model = glm::scale(model, glm::vec3(2.0f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, 3.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMapContainer);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();



    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteMap);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(light.position.x, light.position.y, light.position.z));
    model = glm::scale(model, glm::vec3(0.1f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderCube();
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    if (cubeVAO == 0)
    {
        std::vector<float>vertices = genBox();
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
unsigned int roomCubeVAO = 0;
unsigned int roomCubeVBO = 0;
void renderRoomCube()
{
    if (roomCubeVAO == 0)
    {
        std::vector<float>vertices = genRoomBox();
        glGenVertexArrays(1, &roomCubeVAO);
        glGenBuffers(1, &roomCubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, roomCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindVertexArray(roomCubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(roomCubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        light.position.y += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        light.position.y -= deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        light.position.x -= deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        light.position.x += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        light.position.z -= deltaTime;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        light.position.z += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        light.times += 0.001;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        light.times -= 0.001;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (!glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))return;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
std::vector<float> genBox() {
    std::vector<float> Box = {
// 背面
      - 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 左下
       1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // 右上
       1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // 右下         
       1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // 右上
      -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 左下
      -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // 左上
        // 前面
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 左下
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // 右下
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // 右上
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // 右上
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // 左上
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 左下
        // 左面
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 左上
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // 左下
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 右下
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 右下
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 左上
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 左上
        // 右面
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 右上
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 右下
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // 右上         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // 右下
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // 右上
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下     
        // 下面
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 右上
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // 左上
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 左下
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 左下
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // 右下
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 右上
        // 上面
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // 左上
         1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // 右下
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // 右上     
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // 右下
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // 左上
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,0.0f, 0.0f  // 左下 
    };
    return Box;
}
std::vector<float> genRoomBox() {
    std::vector<float> Box = {
        // 背面
              -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 左下
               1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 右上
               1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 右下         
               1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 右上
              -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 左下
              -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // 左上
              // 前面
              -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 左下
               1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 右下
               1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 右上
               1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,0.0f, 0.0f, // 右上
              -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 左上
              -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // 左下
              // 左面
              -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 左上
              -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 左下
              -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下
              -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下
              -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 左上
              -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 左上
              // 右面
               1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右上
               1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下
               1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右上         
               1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下
               1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右上
               1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // 右下     
               // 下面
               -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 右上
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // 左上
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 左下
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 左下
               -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // 右下
               -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 右上
               // 上面
               -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 左上
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 右下
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 右上     
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 右下
               -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, // 左上
               -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,0.0f, 0.0f  // 左下 
    };
    return Box;
}