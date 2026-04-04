#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "Model.h"
#include <Camera.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cmath>
#define M_PI 3.14159265358979323846
#define M_E 2.71828182845904523536

void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(vector<std::string> faces);
void renderQuad();
std::vector<glm::vec2> getMipmapResolutions(int sizeX, int sizeY);
std::vector<std::vector<glm::vec4>> getPhotonBufferFillers(std::vector<glm::vec2>& mipmapResolutions);
unsigned int* getFilledPhotonBuffers(std::vector<glm::vec2>& mipmapResolutions, std::vector<std::vector<glm::vec4>>& fillers);
float* getGaussianFilterKernel(int sizeX, int sizeY, float standardDeviation);
float calculateSolidAngle();

const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 1024;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool settingsEnabled = false;
bool debugBackfaceNormals = false;
bool dTildeFix = false;

float lightPosition[3] = {5.0f, 9.5f, 7.5f};
float lightColor[3] = {1.0f, 1.0f, 1.0f};
float lightDirection[3] = { 0.0f, 0.0f, -1.0f };
float lightCutOff = 12.5f;
float lightOuterCutOff = 17.5f;
int currentDebugOption = 0;
int currentGapFillingAlgorithm = 0;
const char* debugOptions[] = { "DISABLED", "POSITION", "NORMALS", "REVERSED_DEPTH", "DEPTH", "NORMAL_BACK", "SHADOW_MAP", "PHOTON_POSITIONS", "DISPERSION_MAP", "PHOTONS", "BLURRED_DISPERSION_MAP", "BLANCHETTE_DISPERSION_MAP", "PHOTONS_TO_TEXTURE"};
float indexOfRefraction = 1.0f;
float secondIndexOfRefraction = 1.0f;
float thetaIClampValue = 3.1416f;
int dispersedPhotonsMapLevel = 0;
bool ccdOptimizationEnabled = true;
float testObjectPosition[3] = {5.4f, 8.0f, -2.0f};
float testObjectRotationX = 0.0f;
float testObjectRotationY = 0.0f;
float testObjectRotationZ = 0.0f;
bool photonCCDBasedDistanceCalculationEnabled = true;
const char* gapFillingAlgorithms[] = {"DISTANCE_BASED", "BLANCHETTE"};
float blanchetteFetchRadius = 5.0f;
int blanchetteSamplesCount = 10;
int blanchetteHitCountThreshold = 5;
float blanchetteColorThreshold = 0.1f;
bool useCustomSolidAngle = false;
float customSolidAngle = 0.000003f;
float indexOfRefractionStep = 0.1f;
float secondIndexOfRefractionStep = 0.1f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dispersion", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    float solidAngleForCurrentResolution = calculateSolidAngle();

    Shader basicShader("shaders/basic.vert", "shaders/basic.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader geometryPassShader("shaders/geometryPass.vert", "shaders/geometryPass.frag");
    Shader lightGeometryPassShader("shaders/lightGeometryPass.vert", "shaders/lightGeometryPass.frag");
    Shader refractionFirstPassShader("shaders/refractionFirstPass.vert", "shaders/refractionFirstPass.frag");
    Shader refractionSecondPassShader("shaders/refractionSecondPass.vert", "shaders/refractionSecondPass.frag");
    Shader lightingPassShader("shaders/lightingPass.vert", "shaders/lightingPass.frag");
    Shader renderResultTestShader("shaders/renderResultTest.vert", "shaders/renderResultTest.frag");
    Shader depthMapShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    Shader photonProcessingShader("shaders/photonProcessing.vert", "shaders/photonProcessing.frag", "shaders/photonProcessing.geom", true);
    Shader photonSplattingShader("shaders/photonSplatting.vert", "shaders/photonSplatting.frag", "shaders/photonSplatting.geom");
    Shader dispersionMapCreationShader("shaders/dispersionMapCreation.vert", "shaders/dispersionMapCreation.frag");
    Shader gaussianFilterShader("shaders/gaussianFilter.vert", "shaders/gaussianFilter.frag");
    Shader blanchetteGapFillingShader("shaders/blanchetteGapFilling.vert", "shaders/blanchetteGapFilling.frag");
    Shader photonsToTextureShader("shaders/photonsToTexture.vert", "shaders/photonsToTexture.frag");

    photonsToTextureShader.use();
    photonsToTextureShader.setInt("photonBuffer", 0);

    blanchetteGapFillingShader.use();
    blanchetteGapFillingShader.setInt("dipsersionMapWithGaps", 0);
    blanchetteGapFillingShader.setVec2("viewportSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));

    dispersionMapCreationShader.use();
    dispersionMapCreationShader.setInt("dispersedPhotons", 0);

    float* gaussianFilterKernel = getGaussianFilterKernel(21, 21, 5.0f);
    gaussianFilterShader.use();
    gaussianFilterShader.setInt("imageToBlur", 0);
    gaussianFilterShader.setInt("viewportSize", SCR_WIDTH);
    gaussianFilterShader.setInt("kernelSize", 21);
    gaussianFilterShader.setFloatX("coefs", 441, gaussianFilterKernel);

    photonProcessingShader.use();
    photonProcessingShader.setInt("photonPositions", 0);

    float* coefs3x3 = getGaussianFilterKernel(3, 3, 1.0f);
    float* coefs7x7 = getGaussianFilterKernel(7, 7, 1.0f);
    float* coefs11x11 = getGaussianFilterKernel(11, 11, 1.0f);
    float* coefs15x15 = getGaussianFilterKernel(15, 15, 1.0f);
    photonSplattingShader.use();
    photonSplattingShader.setFloatX("coefs3x3", 9, coefs3x3);
    photonSplattingShader.setFloatX("coefs7x7", 49, coefs7x7);
    photonSplattingShader.setFloatX("coefs11x11", 121, coefs11x11);
    photonSplattingShader.setFloatX("coefs15x15", 225, coefs15x15);
    photonSplattingShader.setInt("redPhotonPositions", 0);
    photonSplattingShader.setInt("greenPhotonPositions", 1);
    photonSplattingShader.setInt("bluePhotonPositions", 2);
    photonSplattingShader.setInt("orangePhotonPositions", 3);
    photonSplattingShader.setInt("yellowPhotonPositions", 4);
    photonSplattingShader.setInt("indigoPhotonPositions", 5);
    photonSplattingShader.setInt("purplePhotonPositions", 6);
    photonSplattingShader.setInt("photonsTexture", 7);
    photonSplattingShader.setVec2("viewportSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));

    renderResultTestShader.use();
    renderResultTestShader.setInt("renderResult", 0);
    renderResultTestShader.setInt("arrayRenderResult", 1);

    //Model refractiveBox("models/box.obj");
    //Model solidBox("models/box.obj");
    //Model bunny("models/bunny.obj");
    Model sphere("models/dragonScaled.obj");
    Model room("models/roomScaled.obj");

    float skyboxVertices[] = {     
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
    {
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    lightingPassShader.use();
    lightingPassShader.setInt("gPosition", 0);
    lightingPassShader.setInt("gNormal", 1);
    lightingPassShader.setInt("gAlbedoSpec", 2);
    lightingPassShader.setInt("shadowMap", 3);
    lightingPassShader.setInt("causticMap", 4);

    unsigned int renderResultBuffer;
    glGenFramebuffers(1, &renderResultBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, renderResultBuffer);
    unsigned int afterLightingPass, resultDepth;

    glGenTextures(1, &afterLightingPass);
    glBindTexture(GL_TEXTURE_2D, afterLightingPass);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, afterLightingPass, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glGenTextures(1, &resultDepth);
    glBindTexture(GL_TEXTURE_2D, resultDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, resultDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int refractionBuffer;
    glGenFramebuffers(1, &refractionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionBuffer);
    unsigned int rReversedDepth, rNormalBack;

    glGenTextures(1, &rNormalBack);
    glBindTexture(GL_TEXTURE_2D, rNormalBack);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rNormalBack, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glGenTextures(1, &rReversedDepth);
    glBindTexture(GL_TEXTURE_2D, rReversedDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rReversedDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int refractionSecondBuffer;
    glGenFramebuffers(1, &refractionSecondBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, refractionSecondBuffer);
    unsigned int r2Result, r2Depth;

    glGenTextures(1, &r2Result);
    glBindTexture(GL_TEXTURE_2D, r2Result);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r2Result, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glGenTextures(1, &r2Depth);
    glBindTexture(GL_TEXTURE_2D, r2Depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r2Depth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int depthMapBuffer;
    glGenFramebuffers(1, &depthMapBuffer);
    unsigned int depthMap;

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int lightGBuffer;
    glGenFramebuffers(1, &lightGBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, lightGBuffer);
    unsigned int lgPositions;

    glGenTextures(1, &lgPositions);
    glBindTexture(GL_TEXTURE_2D, lgPositions);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lgPositions, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    unsigned int lightGBufferRboDepth;
    glGenRenderbuffers(1, &lightGBufferRboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, lightGBufferRboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lightGBufferRboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int photonBuffer;
    glGenFramebuffers(1, &photonBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, photonBuffer);
    unsigned int photonLocations;
    unsigned int redPhotonLocations, greenPhotonLocations, bluePhotonLocations,
                 orangePhotonLocations, yellowPhotonLocations, indigoPhotonLocations,
                 purplePhotonLocations;

    glGenTextures(1, &photonLocations);
    glBindTexture(GL_TEXTURE_2D, photonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, photonLocations, 0);

    glGenTextures(1, &redPhotonLocations);
    glBindTexture(GL_TEXTURE_2D, redPhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, redPhotonLocations, 0);

    glGenTextures(1, &greenPhotonLocations);
    glBindTexture(GL_TEXTURE_2D, greenPhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, greenPhotonLocations, 0);

    glGenTextures(1, &bluePhotonLocations);
    glBindTexture(GL_TEXTURE_2D, bluePhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, bluePhotonLocations, 0);

    glGenTextures(1, &orangePhotonLocations);
    glBindTexture(GL_TEXTURE_2D, orangePhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, orangePhotonLocations, 0);

    glGenTextures(1, &yellowPhotonLocations);
    glBindTexture(GL_TEXTURE_2D, yellowPhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, yellowPhotonLocations, 0);

    glGenTextures(1, &indigoPhotonLocations);
    glBindTexture(GL_TEXTURE_2D, indigoPhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_2D, indigoPhotonLocations, 0);

    glGenTextures(1, &purplePhotonLocations);
    glBindTexture(GL_TEXTURE_2D, purplePhotonLocations);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, GL_TEXTURE_2D, purplePhotonLocations, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    unsigned int photonBufferRboDepth;
    glGenRenderbuffers(1, &photonBufferRboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, photonBufferRboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, photonBufferRboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int lightRefractionBuffer;
    glGenFramebuffers(1, &lightRefractionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, lightRefractionBuffer);
    unsigned int lRReversedDepth, lRNormalBack;

    glGenTextures(1, &lRNormalBack);
    glBindTexture(GL_TEXTURE_2D, lRNormalBack);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lRNormalBack, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glGenTextures(1, &lRReversedDepth);
    glBindTexture(GL_TEXTURE_2D, lRReversedDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, lRReversedDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int causticsBuffer;
    glGenFramebuffers(1, &causticsBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, causticsBuffer);
    unsigned int causticMap;

    glGenTextures(1, &causticMap);
    glBindTexture(GL_TEXTURE_2D, causticMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColorCM[] = { 0.0, 0.0, 0.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorCM);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, causticMap, 0);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    unsigned int causticsBufferRboDepth;
    glGenRenderbuffers(1, &causticsBufferRboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, causticsBufferRboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, causticsBufferRboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int photonsVBO, photonsVAO, additionalPhotonsVBO;
    bool useAdditionalPhotonsVBO = false;
    glGenVertexArrays(1, &photonsVAO);
    glGenBuffers(1, &photonsVBO);
    glGenBuffers(1, &additionalPhotonsVBO);

    float initiatingPhoton[]{ 0.0f, 0.0f, 0.0f, 1.0f };

    int fps = 0;
    float fpsRefreshTimer = 0.0f;
    
    std::vector<glm::vec2> mipmapResolutionsToProcess = getMipmapResolutions(SCR_WIDTH, SCR_HEIGHT);
    std::vector<std::vector<glm::vec4>> photonBufferFillers = getPhotonBufferFillers(mipmapResolutionsToProcess);

    unsigned int* filledPhotonBuffers = getFilledPhotonBuffers(mipmapResolutionsToProcess, photonBufferFillers);

    unsigned int dispersionBuffer;
    glGenFramebuffers(1, &dispersionBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, dispersionBuffer);
    unsigned int dispersedPhotonsMap;

    glGenTextures(1, &dispersedPhotonsMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, dispersedPhotonsMap);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, mipmapResolutionsToProcess.size(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColorDM[] = { 0.0, 0.0, 0.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColorDM);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dispersedPhotonsMap, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int dispersionMapBuffer;
    glGenFramebuffers(1, &dispersionMapBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, dispersionMapBuffer);
    unsigned int dispersionMap, blurredDispersionMap, blanchetteDispersionMap;

    glGenTextures(1, &dispersionMap);
    glBindTexture(GL_TEXTURE_2D, dispersionMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorDM);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dispersionMap, 0);

    glGenTextures(1, &blurredDispersionMap);
    glBindTexture(GL_TEXTURE_2D, blurredDispersionMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorDM);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, blurredDispersionMap, 0);

    glGenTextures(1, &blanchetteDispersionMap);
    glBindTexture(GL_TEXTURE_2D, blanchetteDispersionMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorDM);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, blanchetteDispersionMap, 0);

    unsigned int dispersionAttachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    unsigned int dispersionMapBufferRboDepth;
    glGenRenderbuffers(1, &dispersionMapBufferRboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, dispersionMapBufferRboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dispersionMapBufferRboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int blanchetteBuffer;
    glGenFramebuffers(1, &blanchetteBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, blanchetteBuffer);
    unsigned int blanchetteGapFillingResult;

    glGenTextures(1, &blanchetteGapFillingResult);
    glBindTexture(GL_TEXTURE_2D, blanchetteGapFillingResult);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blanchetteGapFillingResult, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int photonsToTextureBuffer;
    glGenFramebuffers(1, &photonsToTextureBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, photonsToTextureBuffer);
    unsigned int photonsToTexture;

    glGenTextures(1, &photonsToTexture);
    glBindTexture(GL_TEXTURE_2D, photonsToTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColorDM);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, photonsToTexture, 0);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int emittedLightQO;
    glGenQueries(1, &emittedLightQO);
    unsigned int emittedLight;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        fpsRefreshTimer += deltaTime;
        if (fpsRefreshTimer >= 0.1f)
        {
            fps = 1.0f / deltaTime;
            fpsRefreshTimer  = 0.0f;
        }
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 solidBoxTransform = glm::mat4(1.0f);
        glm::mat4 anotherSolidBoxTransform = glm::translate(solidBoxTransform, glm::vec3(1.0f, 0.4f, -3.0f));
        glm::mat4 roomTransform = glm::translate(solidBoxTransform, glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4 bunnyTransform = glm::mat4(1.0f);
        bunnyTransform = glm::translate(bunnyTransform, glm::vec3(-3.0f, 0.0f, 2.0f));
        bunnyTransform = glm::scale(bunnyTransform, glm::vec3(5.0f, 5.0f, 5.0f));
        glm::mat4 refractiveBoxTransform = glm::mat4(1.0f);
        refractiveBoxTransform = glm::translate(refractiveBoxTransform, glm::vec3(3.0f, 0.0f, 2.0f));
        glm::mat4 sphereTransform = glm::mat4(1.0f);
        sphereTransform = glm::translate(sphereTransform, glm::vec3(testObjectPosition[0], testObjectPosition[1], testObjectPosition[2]));
        sphereTransform = glm::rotate(sphereTransform, glm::radians(testObjectRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        sphereTransform = glm::rotate(sphereTransform, glm::radians(testObjectRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        sphereTransform = glm::rotate(sphereTransform, glm::radians(testObjectRotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 bigSphereTransform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
        bigSphereTransform = glm::translate(bigSphereTransform, glm::vec3(30.0f, 0.0f, 4.0f));

        glm::mat4 lightProjection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::vec3 lightPositionVec(lightPosition[0], lightPosition[1], lightPosition[2]);
        glm::vec3 lightDirectionVec(lightDirection[0], lightDirection[1], lightDirection[2]);
        lightDirectionVec = glm::normalize(lightDirectionVec);
        glm::mat4 lightView = glm::lookAt(lightPositionVec, lightPositionVec + lightDirectionVec, glm::vec3(0.0, 1.0, 0.0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

#pragma region Shadow map generation
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapBuffer);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthMapShader.use();
        depthMapShader.setMat4("projection", lightProjection);
        depthMapShader.setMat4("view", lightView);

        depthMapShader.setMat4("model", solidBoxTransform);
        //solidBox.Draw(depthMapShader);
        depthMapShader.setMat4("model", anotherSolidBoxTransform);
        //solidBox.Draw(depthMapShader);
        depthMapShader.setMat4("model", sphereTransform);
        sphere.Draw(depthMapShader);
        depthMapShader.setMat4("model", roomTransform);
        room.Draw(depthMapShader);
        depthMapShader.setMat4("model", bigSphereTransform);
        sphere.Draw(depthMapShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Caustic mapping first pass
        glBindFramebuffer(GL_FRAMEBUFFER, lightGBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightGeometryPassShader.use();
        lightGeometryPassShader.setMat4("projection", lightProjection);
        lightGeometryPassShader.setMat4("view", lightView);
        lightGeometryPassShader.setMat4("model", solidBoxTransform);
        //solidBox.Draw(lightGeometryPassShader);
        lightGeometryPassShader.setMat4("model", anotherSolidBoxTransform);
        //solidBox.Draw(lightGeometryPassShader);
        lightGeometryPassShader.setMat4("model", roomTransform);
        room.Draw(lightGeometryPassShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, lightRefractionBuffer);
        glClearDepth(0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_GREATER);
        refractionFirstPassShader.use();
        refractionFirstPassShader.setMat4("projection", lightProjection);
        refractionFirstPassShader.setMat4("view", lightView);
        refractionFirstPassShader.setMat4("model", sphereTransform);
        sphere.Draw(refractionFirstPassShader);
        refractionFirstPassShader.setMat4("model", bigSphereTransform);
        sphere.Draw(refractionFirstPassShader);
        glDepthFunc(GL_LESS);
        glClearDepth(1.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        for (int monochromaticLightIndex = 1; monochromaticLightIndex < 8; monochromaticLightIndex++)
        {
            float indexOfRefractionForWavelength, secondIndexOfRefractionForWavelength;
            switch (monochromaticLightIndex)
            {
            case 1:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 6.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 6.0f);
                break;
            }
            case 2:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 3.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 3.0f);
                break;
            }
            case 3:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 2.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 2.0f);
                break;
            }
            case 4:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 5.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 5.0f);
                break;
            }
            case 5:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 4.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 4.0f);
                break;
            }
            case 6:
            {
                indexOfRefractionForWavelength = indexOfRefraction + (indexOfRefractionStep * 1.0f);
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction + (secondIndexOfRefractionStep * 1.0f);
                break;
            }
            case 7:
            {
                indexOfRefractionForWavelength = indexOfRefraction;
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction;
                break;
            }
            default:
            {
                indexOfRefractionForWavelength = indexOfRefraction;
                secondIndexOfRefractionForWavelength = secondIndexOfRefraction;
                break;
            }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, photonBuffer);

            glDrawBuffer(GL_COLOR_ATTACHMENT0 + monochromaticLightIndex);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, lightGBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, photonBuffer);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, photonBuffer);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, lRReversedDepth);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, lRNormalBack);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, lgPositions);
            refractionSecondPassShader.use();
            refractionSecondPassShader.setVec3("viewPos", lightPositionVec);
            refractionSecondPassShader.setVec3("cameraLookAtVector", lightDirectionVec);
            refractionSecondPassShader.setVec2("viewportSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
            refractionSecondPassShader.setMat4("projection", lightProjection);
            refractionSecondPassShader.setMat4("view", lightView);
            refractionSecondPassShader.setBool("debugBackfaceNormals", debugBackfaceNormals);
            refractionSecondPassShader.setBool("dTildeFix", dTildeFix);
            refractionSecondPassShader.setMat4("viewProjection", lightProjection * lightView);
            refractionSecondPassShader.setInt("BackfaceZBuf", 0);
            refractionSecondPassShader.setInt("BackfaceNormals", 1);
            refractionSecondPassShader.setInt("EnvironmentMap", 2);
            refractionSecondPassShader.setFloat("indexOfRefraction", indexOfRefractionForWavelength);
            refractionSecondPassShader.setFloat("secondIndexOfRefraction", secondIndexOfRefractionForWavelength);
            refractionSecondPassShader.setFloat("thetaIClampValue", thetaIClampValue);
            refractionSecondPassShader.setMat4("model", sphereTransform);
            sphere.Draw(refractionSecondPassShader);
            refractionSecondPassShader.setMat4("model", bigSphereTransform);
            sphere.Draw(refractionSecondPassShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, redPhotonLocations);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindFramebuffer(GL_FRAMEBUFFER, dispersionBuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(photonsVAO);

        glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(initiatingPhoton), initiatingPhoton, GL_DYNAMIC_COPY);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        useAdditionalPhotonsVBO = false;

        glEnable(GL_RASTERIZER_DISCARD);
        for (int resolutionIndex = mipmapResolutionsToProcess.size() - 2; resolutionIndex >= 0; resolutionIndex--)
        {
            glm::vec2 previousMipmapResolution = mipmapResolutionsToProcess[resolutionIndex + 1];
            glm::vec2 mipmapResolution = mipmapResolutionsToProcess[resolutionIndex];

            glViewport(0, 0,
                mipmapResolution.x,
                mipmapResolution.y);

            photonProcessingShader.use();
            photonProcessingShader.setFloat("mipmapLevel", resolutionIndex + 1);
            photonProcessingShader.setVec2("photonChildOffset", glm::vec2(1.0f / mipmapResolution.x,
                1.0f / mipmapResolution.x));
            photonProcessingShader.setVec2("viewportSize", mipmapResolution);

            //std::vector<glm::vec4>* photonBufferFiller = &photonBufferFillers[resolutionIndex];
            //&(*photonBufferFiller)[0]

            if (!useAdditionalPhotonsVBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
            }
            else
            {
                glBindBuffer(GL_ARRAY_BUFFER, additionalPhotonsVBO);
            }
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            if (!useAdditionalPhotonsVBO)
            {
                /*glBindBuffer(GL_ARRAY_BUFFER, additionalPhotonsVBO);
                glBufferData(GL_ARRAY_BUFFER, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float), &photonBufferFillers[resolutionIndex][0], GL_DYNAMIC_COPY);
                glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, additionalPhotonsVBO);
                useAdditionalPhotonsVBO = true;*/

                glBindBuffer(GL_ARRAY_BUFFER, additionalPhotonsVBO);
                glBufferData(GL_ARRAY_BUFFER, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float), NULL, GL_DYNAMIC_COPY);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_COPY_READ_BUFFER, filledPhotonBuffers[resolutionIndex]);
                glBindBuffer(GL_COPY_WRITE_BUFFER, additionalPhotonsVBO);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float));
                glBindBuffer(GL_COPY_READ_BUFFER, 0);
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                glBindBuffer(GL_ARRAY_BUFFER, additionalPhotonsVBO);
                glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, additionalPhotonsVBO);
                useAdditionalPhotonsVBO = true;
            }
            else
            {
                /*glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
                glBufferData(GL_ARRAY_BUFFER, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float), &photonBufferFillers[resolutionIndex][0], GL_DYNAMIC_COPY);
                glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, photonsVBO);
                useAdditionalPhotonsVBO = false;*/

                glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
                glBufferData(GL_ARRAY_BUFFER, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float), NULL, GL_DYNAMIC_COPY);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_COPY_READ_BUFFER, filledPhotonBuffers[resolutionIndex]);
                glBindBuffer(GL_COPY_WRITE_BUFFER, photonsVBO);
                glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float));
                glBindBuffer(GL_COPY_READ_BUFFER, 0);
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
                glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, photonsVBO);
                useAdditionalPhotonsVBO = false;
            }

            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, previousMipmapResolution.x * previousMipmapResolution.y);
            glEndTransformFeedback();

            glFlush();

            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
        }
        glDisable(GL_RASTERIZER_DISCARD);

        if (!useAdditionalPhotonsVBO)
        {
            glBindBuffer(GL_ARRAY_BUFFER, photonsVBO);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, additionalPhotonsVBO);
        }
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        photonSplattingShader.use();
        photonSplattingShader.setFloat("solidAngle", solidAngleForCurrentResolution);
        photonSplattingShader.setMat4("viewProj", lightProjection * lightView);
        photonSplattingShader.setMat4("view", lightView);
        photonSplattingShader.setBool("ccdOptimizationEnabled", ccdOptimizationEnabled);
        photonSplattingShader.setBool("photonCCDBasedCalculationEnabled", photonCCDBasedDistanceCalculationEnabled);
        photonSplattingShader.setBool("blanchetteGapFillingOverride", currentGapFillingAlgorithm == 1 ? true : false);
        photonSplattingShader.setBool("useCustomSolidAngle", useCustomSolidAngle);
        photonSplattingShader.setFloat("customSolidAngle", customSolidAngle);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, redPhotonLocations);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, greenPhotonLocations);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, bluePhotonLocations);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, orangePhotonLocations);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, yellowPhotonLocations);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, indigoPhotonLocations);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, purplePhotonLocations);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, photonsToTexture);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glBeginQuery(GL_PRIMITIVES_GENERATED, emittedLightQO);

        glDrawArrays(GL_POINTS, 0, SCR_WIDTH * SCR_HEIGHT);

        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectuiv(emittedLightQO, GL_QUERY_RESULT, &emittedLight);

        /*
        glBindFramebuffer(GL_FRAMEBUFFER, photonsToTextureBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, purplePhotonLocations);
        photonsToTextureShader.use();
        photonsToTextureShader.setMat4("viewProj", lightSpaceMatrix);

        glDrawArrays(GL_POINTS, 0, SCR_WIDTH * SCR_HEIGHT);
        */

        glBindFramebuffer(GL_FRAMEBUFFER, dispersionMapBuffer);
        glDrawBuffers(3, dispersionAttachments);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, dispersedPhotonsMap);
        dispersionMapCreationShader.use();
        renderQuad();
        if (currentGapFillingAlgorithm == 0)
        {
            glBindTexture(GL_TEXTURE_2D, dispersionMap);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            gaussianFilterShader.use();
            renderQuad();
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, dispersionMap);
            glDrawBuffer(GL_COLOR_ATTACHMENT2);
            blanchetteGapFillingShader.use();
            blanchetteGapFillingShader.setFloat("fetchRadius", blanchetteFetchRadius);
            blanchetteGapFillingShader.setFloat("colorThreshold", blanchetteColorThreshold);
            blanchetteGapFillingShader.setInt("samplesCount", blanchetteSamplesCount);
            blanchetteGapFillingShader.setInt("hitCountThreshold", blanchetteHitCountThreshold);
            renderQuad();
            glBindTexture(GL_TEXTURE_2D, blanchetteDispersionMap);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            gaussianFilterShader.use();
            renderQuad();
        }

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

#pragma endregion

#pragma region Geometry pass
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometryPassShader.use();
        geometryPassShader.setMat4("projection", projection);
        geometryPassShader.setMat4("view", view);
        geometryPassShader.setMat4("model", solidBoxTransform);
        //solidBox.Draw(geometryPassShader);
        geometryPassShader.setMat4("model", anotherSolidBoxTransform);
        //solidBox.Draw(geometryPassShader);
        geometryPassShader.setMat4("model", roomTransform);
        room.Draw(geometryPassShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Lighting pass
        glBindFramebuffer(GL_FRAMEBUFFER, renderResultBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightingPassShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, blurredDispersionMap);

        glm::vec3 lightColorVec(lightColor[0], lightColor[1], lightColor[2]);
        lightingPassShader.setVec3("light.Position", lightPositionVec);
        lightingPassShader.setVec3("light.Color", lightColorVec);
        const float constant = 1.0f;
        const float linear = 0.022f;
        const float quadratic = 0.0019f;
        lightingPassShader.setFloat("light.Linear", linear);
        lightingPassShader.setFloat("light.Quadratic", quadratic);
        const float maxBrightness = std::fmaxf(std::fmaxf(lightColorVec.r, lightColorVec.g), lightColorVec.b);
        lightingPassShader.setVec3("light.Direction", lightDirectionVec);
        lightingPassShader.setFloat("light.CutOff", glm::cos(glm::radians(lightCutOff)));
        lightingPassShader.setFloat("light.OuterCutOff", glm::cos(glm::radians(lightOuterCutOff)));
        lightingPassShader.setMat4("light.SpaceMatrix", lightSpaceMatrix);
        
        lightingPassShader.setVec3("viewPos", camera.Position);

        renderQuad();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderResultBuffer);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, renderResultBuffer);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 viewForSkybox = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", viewForSkybox);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Refraction first pass
        glBindFramebuffer(GL_FRAMEBUFFER, refractionBuffer);
        glClearDepth(0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_GREATER);
        refractionFirstPassShader.use();
        refractionFirstPassShader.setMat4("projection", projection);
        refractionFirstPassShader.setMat4("view", view);
        refractionFirstPassShader.setMat4("model", refractiveBoxTransform);
        //refractiveBox.Draw(refractionFirstPassShader);
        refractionFirstPassShader.setMat4("model", bigSphereTransform);
        //bunny.Draw(refractionFirstPassShader);
        refractionFirstPassShader.setMat4("model", sphereTransform);
        sphere.Draw(refractionFirstPassShader);
        refractionFirstPassShader.setMat4("model", bigSphereTransform);
        sphere.Draw(refractionFirstPassShader);
        glDepthFunc(GL_LESS);
        glClearDepth(1.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Refraction second pass
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, refractionSecondBuffer);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderResultBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, refractionSecondBuffer);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, refractionSecondBuffer);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rReversedDepth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rNormalBack);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, afterLightingPass);
        refractionSecondPassShader.use();
        refractionSecondPassShader.setVec3("viewPos", camera.Position);
        refractionSecondPassShader.setVec3("cameraLookAtVector", camera.Front);
        refractionSecondPassShader.setVec2("viewportSize", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        refractionSecondPassShader.setMat4("projection", projection);
        refractionSecondPassShader.setMat4("view", view);
        refractionSecondPassShader.setBool("debugBackfaceNormals", debugBackfaceNormals);
        refractionSecondPassShader.setBool("dTildeFix", dTildeFix);
        //refractionSecondPassShader.setMat4("model", model);
        refractionSecondPassShader.setMat4("viewProjection", projection* view);
        refractionSecondPassShader.setInt("BackfaceZBuf", 0);
        refractionSecondPassShader.setInt("BackfaceNormals", 1);
        refractionSecondPassShader.setInt("EnvironmentMap", 2);
        refractionSecondPassShader.setMat4("model", refractiveBoxTransform);
        refractionSecondPassShader.setFloat("indexOfRefraction", indexOfRefraction);
        refractionSecondPassShader.setFloat("secondIndexOfRefraction", secondIndexOfRefraction);
        refractionSecondPassShader.setFloat("thetaIClampValue", thetaIClampValue);
        //refractiveBox.Draw(refractionSecondPassShader);
        refractionSecondPassShader.setMat4("model", bigSphereTransform);
        //bunny.Draw(refractionSecondPassShader);
        refractionSecondPassShader.setMat4("model", sphereTransform);
        sphere.Draw(refractionSecondPassShader);
        refractionSecondPassShader.setMat4("model", bigSphereTransform);
        sphere.Draw(refractionSecondPassShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

#pragma region Deferred shading debugging
        renderResultTestShader.use();
        renderResultTestShader.setBool("depthDebug", false);
        renderResultTestShader.setBool("arrayTextureDebug", false);
        glActiveTexture(GL_TEXTURE0);
        switch (currentDebugOption)
        {
        case 0:
        {
            glBindTexture(GL_TEXTURE_2D, r2Result);
            break;
        }
        case 1:
        {
            glBindTexture(GL_TEXTURE_2D, gPosition);
            break;
        }
        case 2:
        {
            glBindTexture(GL_TEXTURE_2D, gNormal);
            break;
        }
        case 3:
            glBindTexture(GL_TEXTURE_2D, rReversedDepth);
            renderResultTestShader.setBool("depthDebug", true);
            break;
        case 4:
            break;
        case 5:
            glBindTexture(GL_TEXTURE_2D, rNormalBack);
            break;
        case 6:
            glBindTexture(GL_TEXTURE_2D, depthMap);
            renderResultTestShader.setBool("depthDebug", true);
            break;
        case 7:
            glBindTexture(GL_TEXTURE_2D, redPhotonLocations);
            break;
        case 8:
            glBindTexture(GL_TEXTURE_2D, dispersionMap);
            break;
        case 9:
            renderResultTestShader.setBool("arrayTextureDebug", true);
            renderResultTestShader.setFloat("arrayTextureLayer", dispersedPhotonsMapLevel);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D_ARRAY, dispersedPhotonsMap);
            break;
        case 10:
            glBindTexture(GL_TEXTURE_2D, blurredDispersionMap);
            break;
        case 11:
            glBindTexture(GL_TEXTURE_2D, blanchetteDispersionMap);
            break;
        case 12:
            glBindTexture(GL_TEXTURE_2D, photonsToTexture);
            break;
        default:
        {
            break;
        }
        }
#pragma endregion

        renderQuad();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, refractionSecondBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (settingsEnabled)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGui::Begin("Settings");

                ImGui::Text((std::to_string(fps) + " FPS").c_str());
                ImGui::Text((std::to_string(emittedLight) + " emitted light").c_str());
                ImGui::Combo("Deferred shading debug", &currentDebugOption, debugOptions, IM_ARRAYSIZE(debugOptions));
                ImGui::InputFloat3("Light position", lightPosition);
                ImGui::InputFloat3("Light direction", lightDirection);
                ImGui::InputFloat3("Test object position", testObjectPosition);
                ImGui::ColorEdit3("Light color", lightColor);
                ImGui::SliderFloat("Light Cut Off", &lightCutOff, 0.0f, 90.0f);
                ImGui::SliderFloat("Light Outer Cut Off", &lightOuterCutOff, 0.0f, 90.0f);
                ImGui::InputFloat("Index of refraction", &indexOfRefraction);
                ImGui::SliderFloat ("Index of refraction step", &indexOfRefractionStep, 0.001f, 0.2f);
                ImGui::InputFloat("Second index of refraction", &secondIndexOfRefraction);
                ImGui::SliderFloat("Second index of refraction step", &secondIndexOfRefractionStep, 0.001f, 0.2f);
                ImGui::InputFloat("ThetaI clamp value", &thetaIClampValue);
                ImGui::Checkbox("Debug backface normals", &debugBackfaceNormals);
                ImGui::Checkbox("D tilde fix", &dTildeFix);
                ImGui::SliderInt("Dispersed photons map level", &dispersedPhotonsMapLevel, 0, 6);
                ImGui::Checkbox("CCD Optimization", &ccdOptimizationEnabled);
                ImGui::InputFloat("Test object rotation X", &testObjectRotationX);
                ImGui::InputFloat("Test object rotation Y", &testObjectRotationY);
                ImGui::SliderFloat("Test object rotation Z", &testObjectRotationZ, -180.0f, 180.0f);
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
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    {
        camera.Position = glm::vec3(lightPosition[0],
                                    lightPosition[1],
                                    lightPosition[2]);
        camera.Front = glm::normalize(glm::vec3(lightDirection[0],
            lightDirection[1],
            lightDirection[2]));
        camera.updateCameraVectors(true);
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        lightPosition[0] = camera.Position.x;
        lightPosition[1] = camera.Position.y;
        lightPosition[2] = camera.Position.z;
        lightDirection[0] = camera.Front.x;
        lightDirection[1] = camera.Front.y;
        lightDirection[2] = camera.Front.z;
    }
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

void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);

    //unsigned int queryObject;
    //glGenQueries(1, &queryObject);

    //unsigned int primitveCount;

    //glBeginQuery(GL_PRIMITIVES_GENERATED, queryObject);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //glEndQuery(GL_PRIMITIVES_GENERATED);

    //glGetQueryObjectuiv(queryObject, GL_QUERY_RESULT, &primitveCount);
    //std::cout << "mamy: " << primitveCount << std::endl;

    glBindVertexArray(0);

}

std::vector<glm::vec2> getMipmapResolutions(int sizeX, int sizeY)
{
    std::vector<glm::vec2> result;

    int currentSizeX = sizeX;
    int currentSizeY = sizeY;
    while (true)
    {
        result.push_back(glm::vec2(currentSizeX, currentSizeY));

        if (currentSizeX == 1 && currentSizeY == 1)
        {
            break;
        }
        if (currentSizeX >= 2)
        {
            currentSizeX /= 2;
        }
        if (currentSizeY >= 2)
        {
            currentSizeY /= 2;
        }
    }

    return result;
}

std::vector<std::vector<glm::vec4>> getPhotonBufferFillers(std::vector<glm::vec2>& mipmapResolutions)
{
    std::vector<std::vector<glm::vec4>> result;

    for (int resolutionIndex = 0; resolutionIndex < mipmapResolutions.size(); resolutionIndex++)
    {
        std::vector<glm::vec4> filler;
        for (int counter = 0; counter < mipmapResolutions[resolutionIndex].x * mipmapResolutions[resolutionIndex].y; counter++)
        {
            filler.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        }
        result.push_back(filler);
    }

    return result;
}

unsigned int* getFilledPhotonBuffers(std::vector<glm::vec2>& mipmapResolutions, std::vector<std::vector<glm::vec4>>& fillers)
{
    unsigned int* result = new unsigned int[mipmapResolutions.size()];
    glGenBuffers(mipmapResolutions.size(), result);

    for (int resolutionIndex = 0; resolutionIndex < mipmapResolutions.size(); resolutionIndex++)
    {
        glm::vec2 mipmapResolution = mipmapResolutions[resolutionIndex];
        glBindBuffer(GL_ARRAY_BUFFER, result[resolutionIndex]);
        glBufferData(GL_ARRAY_BUFFER, mipmapResolution.x * mipmapResolution.y * 4 * sizeof(float), &fillers[resolutionIndex][0], GL_STATIC_DRAW);
    }

    return result;
}

float* getGaussianFilterKernel(int sizeX, int sizeY, float standardDeviation)
{
    int kernelSize = sizeX * sizeY;

    float* result = new float[kernelSize];

    int center = sizeX / 2;
    int offset = sizeX / 2;
    int kernelIndex = 0;
    float sum = 0.0f;
    for (int y = offset; y >= -offset; y--)
    {
        for (int x = -offset; x <= offset; x++)
        {
            result[kernelIndex] = float((1.0f / (2.0f * M_PI * standardDeviation * standardDeviation)) *
                                   pow(M_E, -(((x * x) + (y * y)) / (2.0f * standardDeviation * standardDeviation))));
            sum += result[kernelIndex];
            kernelIndex++;
        }
    }
    for (int i = 0; i < kernelSize; i++)
    {
        result[i] /= sum;
    }

    return result;
}

float calculateZNDC(float r, float x, float y)
{
    return sqrt((r * r) - (x * x) - (y * y));
}

float sgn(float x)
{
    if (x > 0.0f)
    {
        return 1.0f;
    }
    else if (x < 0.0f)
    {
        return -1.0f;
    }
    else
    {
        return 1.0f;
    }
}

glm::vec3 cartesianToSpherical(glm::vec3& cartesian)
{
    glm::vec3 result;

    result.x = sqrt((cartesian.x * cartesian.x) +
                    (cartesian.y * cartesian.y) +
                    (cartesian.z * cartesian.z));
    result.y = acos(cartesian.z / result.x);
    result.z = sgn(cartesian.y) * acos((cartesian.x) / 
              (sqrt((cartesian.x * cartesian.x) + (cartesian.y * cartesian.y))));

    return result;
}

float calculateSolidAngle()
{
    const float r = 1.0f;
    float xNDCStep = 1.0f / SCR_WIDTH;
    float yNDCStep = 1.0f / SCR_HEIGHT;

    glm::vec3 pointsCartesian[4];
    pointsCartesian[0] = glm::vec3(0.0f, 0.0f, calculateZNDC(r, 0.0f, 0.0f));
    pointsCartesian[1] = glm::vec3(0.0f, yNDCStep, calculateZNDC(r, 0.0f, yNDCStep));
    pointsCartesian[2] = glm::vec3(xNDCStep, yNDCStep, calculateZNDC(r, xNDCStep, yNDCStep));
    pointsCartesian[3] = glm::vec3(xNDCStep, 0.0f, calculateZNDC(r, xNDCStep, 0.0f));

    glm::vec3 pointsSpherical[4];
    pointsSpherical[0] = cartesianToSpherical(pointsCartesian[0]);
    pointsSpherical[1] = cartesianToSpherical(pointsCartesian[1]);
    pointsSpherical[2] = cartesianToSpherical(pointsCartesian[2]);
    pointsSpherical[3] = cartesianToSpherical(pointsCartesian[3]);

    float phi1, phi2, theta1, theta2;

    phi1 = pointsSpherical[3].z;
    phi2 = pointsSpherical[1].z;
    theta1 = pointsSpherical[0].y;
    theta2 = pointsSpherical[2].y;

    return ((phi2 - phi1) * (cos(theta1) - cos(theta2)));
}