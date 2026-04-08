#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "RenderPass/DeferredShadingPass.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <string>

#pragma region Window

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 1024;

#pragma endregion

#pragma region Camera

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

#pragma endregion

#pragma region UI

bool settingsEnabled = false;

#pragma endregion

#pragma region Delta time and FPS counting

float deltaTime = 0.0f;
float lastFrame = 0.0f;
int fps = 0;
float fpsRefreshTimer = 0.0f;

#pragma endregion

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main()
{
#pragma region Libraries initialization and window creation
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Light dispersion simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
#pragma endregion

#pragma region OpenGL functions setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
#pragma endregion

#pragma region Scene setup
    SceneGraphNode sceneRoot;
    SceneGraphNode testObject("models/box.obj");
    SceneGraphNode testRoom("models/roomScaled.obj");
    sceneRoot.addChild(&testObject);
    sceneRoot.addChild(&testRoom);
    Spotlight sceneLight
    {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        12.5f,
        17.5f,
        1.0f,
        0.022f,
        0.0019f
    };
#pragma endregion

    DeferredShadingPass deferredShadingPass(SCR_WIDTH, SCR_HEIGHT);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        fpsRefreshTimer += deltaTime;
        if (fpsRefreshTimer >= 0.1f)
        {
            fps = 1.0f / deltaTime;
            fpsRefreshTimer = 0.0f;
        }
        lastFrame = currentFrame;
        
        processInput(window);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        deferredShadingPass.execute(&sceneRoot, camera.GetViewMatrix(), projection, sceneLight);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, deferredShadingPass.getResult().getID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (settingsEnabled)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGui::Begin("Settings");

                ImGui::Text((std::to_string(fps) + " FPS").c_str());
                //ImGui::Text((std::to_string(emittedLight) + " emitted light").c_str());

                /*if (ImGui::CollapsingHeader("Rendering settings"))
                {
                    ImGui::Combo("Rendering debug", &currentDebugOption, debugOptions, IM_ARRAYSIZE(debugOptions));
                    if (currentDebugOption == 9)
                    {
                        ImGui::SliderInt("Dispersion mipmap level", &dispersedPhotonsMapLevel, 0, 6);
                    }

                    ImGui::InputFloat("First interface ratio of indices", &indexOfRefraction);
                    ImGui::SliderFloat("First interface calculation step", &indexOfRefractionStep, 0.001f, 0.2f);
                    ImGui::InputFloat("Second interface ratio of indices", &secondIndexOfRefraction);
                    ImGui::SliderFloat("Second interface calculation step", &secondIndexOfRefractionStep, 0.001f, 0.2f);
                    ImGui::Checkbox("Debug refraction second pass backface normals", &debugBackfaceNormals);
                    ImGui::Checkbox("Refraction second pass backface normals fix", &dTildeFix);
                    ImGui::Checkbox("CCD Optimization", &ccdOptimizationEnabled);
                    switch (currentGapFillingAlgorithm)
                    {
                    case 0:
                    {
                        ImGui::Checkbox("Photon CCD based distance calculation", &photonCCDBasedDistanceCalculationEnabled);
                        break;
                    }
                    case 1:
                    {
                        ImGui::InputFloat("Blanchette fetch radius", &blanchetteFetchRadius);
                        ImGui::InputInt("Blanchette samples count", &blanchetteSamplesCount);
                        ImGui::InputInt("Blanchette hit count threshold", &blanchetteHitCountThreshold);
                        ImGui::SliderFloat("Blanchette color threshold", &blanchetteColorThreshold, 0.0f, 1.0f);
                        break;
                    }
                    default:
                        break;
                    }

                    ImGui::Combo("Gap filling algorithm", &currentGapFillingAlgorithm, gapFillingAlgorithms, IM_ARRAYSIZE(gapFillingAlgorithms));
                    ImGui::Checkbox("Use custom solid angle", &useCustomSolidAngle);
                    if (useCustomSolidAngle)
                    {
                        ImGui::SliderFloat("Custom solid angle", &customSolidAngle, 0.0000001f, 0.000005f, "%.7f");
                    }
                }

                if (ImGui::CollapsingHeader("Scene settings"))
                {
                    ImGui::InputFloat3("Light position", lightPosition);
                    ImGui::InputFloat3("Light direction", lightDirection);
                    ImGui::ColorEdit3("Light color", lightColor);
                    ImGui::SliderFloat("Light Cut Off", &lightCutOff, 0.0f, 90.0f);
                    ImGui::SliderFloat("Light Outer Cut Off", &lightOuterCutOff, 0.0f, 90.0f);
                    ImGui::Text("");
                    ImGui::InputFloat3("Test object position", testObjectPosition);
                    ImGui::SliderFloat("Test object rotation X", &testObjectRotationX, -180.0f, 180.0f);
                    ImGui::SliderFloat("Test object rotation Y", &testObjectRotationY, -180.0f, 180.0f);
                    ImGui::SliderFloat("Test object rotation Z", &testObjectRotationZ, -180.0f, 180.0f);
                }*/

                ImGui::End();
            }
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (settingsEnabled)
    {
        return;
    }
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!settingsEnabled)
    {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

bool ePressed = false;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (!settingsEnabled)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            camera.MovementSpeed = 20.0f;
        }
        else
        {
            camera.MovementSpeed = 2.5f;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        camera.Position = glm::vec3(40.0f, 0.0f, 8.0f);
        camera.Yaw = 0.0f;
        camera.Pitch = 0.0f;
        camera.updateCameraVectors();
    }
    //if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    //{
    //    camera.Position = glm::vec3(lightPosition[0],
    //        lightPosition[1],
    //        lightPosition[2]);
    //    camera.Front = glm::normalize(glm::vec3(lightDirection[0],
    //        lightDirection[1],
    //        lightDirection[2]));
    //    camera.updateCameraVectors(true);
    //}
    //if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    //{
    //    lightPosition[0] = camera.Position.x;
    //    lightPosition[1] = camera.Position.y;
    //    lightPosition[2] = camera.Position.z;
    //    lightDirection[0] = camera.Front.x;
    //    lightDirection[1] = camera.Front.y;
    //    lightDirection[2] = camera.Front.z;
    //}
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        if (!ePressed)
        {
            settingsEnabled = !settingsEnabled;
            if (settingsEnabled)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            ePressed = true;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
    {
        ePressed = false;
    }
}