#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);
void processInput1(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const *path);

bool CircleColision(glm::vec3 c,float r);

void renderQuad();


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool freeMove = false;
bool spotlightOn = true;
bool bg_changen = false;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;
// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = false;

glm::vec3 lastCamPosition;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    glm::vec3 dirLightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 dirLightAmbDiffSpec = glm::vec3(0.3f, 0.3f,0.2f);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 vecCalibrate = glm::vec3(0.0f);
    glm::vec3 vecRotate = glm::vec3(0.0f);
    float fineCalibrate = 0.001f;
    //test
    vector<std::string> faces;
    unsigned int cubemapTexture;
    //-----
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z;
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_DEPTH_TEST);

    //Face Culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);

    //Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    // _____________________________________________________________________________________________________
    Shader ourShader("resources/shaders/directionLight.vs", "resources/shaders/directionLight.fs");
    Shader transparentShader("resources/shaders/directionLight.vs", "resources/shaders/blendingShader.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader shaderBloomFinal("resources/shaders/bloom_final.vs", "resources/shaders/bloom_final.fs");


    // load models
    // ________________________________________________________________________________________________
    //house
    Model ourModel("resources/objects/rockHouse/RockHouse2_1-2.obj",true);
    ourModel.SetShaderTextureNamePrefix("material.");

    //box
    Model boxModel("resources/objects/box/coffre.obj",true);
    boxModel.SetShaderTextureNamePrefix("material.");

    //coin
    Model coinModel("resources/objects/coin/chinese_coin.obj",true);
    coinModel.SetShaderTextureNamePrefix("material.");

    //magic
    Model magicModel("resources/objects/magic/Logo.obj",true);
    magicModel.SetShaderTextureNamePrefix("material.");

    //skull
    Model skullModel("resources/objects/skull/12140_Skull_v3_L2.obj",true);
    skullModel.SetShaderTextureNamePrefix("material.");

    //Bloom efekat _____________________________________________________________________________________________
    // configure framebuffers
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    //Seting skybox vertices
    //____________________________________________________________________________________________
    float skyboxVertices[] = {
            // positions
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

    // skybox VAO
    //_______________________________________________________________________________________________
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures for skybox
    // ______________________________________________________________________________________________
    programState->faces =
            {
                    FileSystem::getPath("resources/textures/skybox1/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/back.jpg"),
            };

    programState->cubemapTexture = loadCubemap(programState->faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    //Point light
    //_____________________________________________________________________________________________________
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-5.6f, 5.0f, 1.7f);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.3f;
    pointLight.linear = 0.8f;
    pointLight.quadratic = 0.4f;

    // shader configuration
    // _______________________________________________________________________________________________
    ourShader.use();
    ourShader.setInt("diffuseTexture", 0);
    transparentShader.use();
    transparentShader.setInt("diffuseTexture", 0);
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    shaderBloomFinal.use();
    shaderBloomFinal.setInt("scene", 0);
    shaderBloomFinal.setInt("bloomBlur", 1);

    // render loop
    // ____________________________________________________________________________________________________
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float time = currentFrame;

        // input
        // ____________________________________________________________________________________
        if(freeMove) {
            processInput(window);
        }else{
            processInput1(window);
            programState->camera.Position.y = 0.7f;
        }

        // render
        // ____________________________________________________________________________________
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //render scene into floating point framebuffer
        // ____________________________________________________________________________________
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // enable shader before setting uniforms
        ourShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // directional light
        //___________________________________________________________________________________________________
        ourShader.setVec3("dirLight.direction", programState->dirLightDir);
        ourShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        ourShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        ourShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));

        ourShader.setVec3("pointLights[0].position", glm::vec3(-0.8f ,0.05f, 2.7f));
        ourShader.setVec3("pointLights[0].ambient", pointLight.ambient);
        ourShader.setVec3("pointLights[0].diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLights[0].specular", pointLight.specular);
        ourShader.setFloat("pointLights[0].constant", pointLight.constant);
        ourShader.setFloat("pointLights[0].linear", pointLight.linear);
        ourShader.setFloat("pointLights[0].quadratic", pointLight.quadratic);

        ourShader.setVec3("pointLights[1].position", glm::vec3(-1.2f ,0.3f, -0.05f));
        ourShader.setVec3("pointLights[1].ambient", pointLight.ambient);
        ourShader.setVec3("pointLights[1].diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLights[1].specular", pointLight.specular);
        ourShader.setFloat("pointLights[1].constant", pointLight.constant);
        ourShader.setFloat("pointLights[1].linear", pointLight.linear);
        ourShader.setFloat("pointLights[1].quadratic", pointLight.quadratic);

        // spotLight
        //___________________________________________________________________________________________________
        if (spotlightOn) {
            ourShader.setVec3("spotLight.position", programState->camera.Position);
            ourShader.setVec3("spotLight.direction", programState->camera.Front);
            ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("spotLight.constant", 1.0f);
            ourShader.setFloat("spotLight.linear", 0.09);
            ourShader.setFloat("spotLight.quadratic", 0.032);
            ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
            ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        }else{
            ourShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }

        // render the loaded models
        //___________________________________________________________________________________________
        //render house
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3 (1.0f,-2.5f,0.0f));
        model = glm::scale(model, glm::vec3(0.7f));
        model = glm::rotate(model,glm::radians(-90.0f), glm::vec3(1.0f ,0.0f, 0.0f));
        model = glm::rotate(model,glm::radians(-47.0f), glm::vec3(0.0f ,0.0f, 1.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        //render box
        glm::mat4 modelBox = glm::mat4(1.0f);
        modelBox = glm::translate(modelBox,glm::vec3(-1.25f, -0.1f, 0.05));
        modelBox = glm::scale(modelBox, glm::vec3(0.06f));
        modelBox = glm::rotate(modelBox,glm::radians(-90.0f), glm::vec3(1.0f ,0.0f, 0.0f));
        modelBox = glm::rotate(modelBox,glm::radians(31.5f), glm::vec3(0.0f ,0.0f, 1.0f));
        ourShader.setMat4("model", modelBox);
        boxModel.Draw(ourShader);

        //render coin
        glm::mat4 modelCoin = glm::mat4(1.0f);
        modelCoin = glm::translate(modelCoin,glm::vec3(-1.4f,0.3f, -0.05f));
        modelCoin = glm::scale(modelCoin, glm::vec3(0.008f));
        modelCoin = glm::rotate(modelCoin,glm::radians(83.0f), glm::vec3(1.0f ,0.0f, 0.0f));
        modelCoin = glm::rotate(modelCoin,glm::radians(180.0f), glm::vec3(0.0f ,1.0f, 0.0f));
        modelCoin = glm::rotate(modelCoin,glm::radians(78.0f), glm::vec3(0.0f ,0.0f, 1.0f));
        modelCoin = glm::rotate(modelCoin, time*5.0f, glm::vec3(0.0f ,1.0f, 1.0f));
        ourShader.setMat4("model", modelCoin);
        coinModel.Draw(ourShader);

        //render magic
        glm::mat4 modelmagic = glm::mat4(1.0f);
        modelmagic = glm::translate(modelmagic,glm::vec3(-1.3f,0.1f,0.f));
        modelmagic = glm::scale(modelmagic, glm::vec3(0.032f));
        modelmagic = glm::rotate(modelmagic,time*5.0f, glm::vec3(0.0f ,1.0f, 0.0f));
        ourShader.setMat4("model", modelmagic);
        magicModel.Draw(ourShader);

        //render skull
        glm::mat4 modelSkull = glm::mat4(1.0f);
        modelSkull = glm::translate(modelSkull,glm::vec3(-1.45f,0.08f,0.4f));
        modelSkull = glm::scale(modelSkull, glm::vec3(0.009f));
        modelSkull = glm::rotate(modelSkull,glm::radians(-90.0f), glm::vec3(1.0f ,0.0f, 0.0f));
        modelSkull = glm::rotate(modelSkull,glm::radians(135.0f), glm::vec3(0.0f ,0.0f, 1.0f));
        ourShader.setMat4("model", modelSkull);
        skullModel.Draw(ourShader);
        
        //setting transparent shader
        //_____________________________________________________________________________________________________________
        transparentShader.use();
        transparentShader.setMat4("projection", projection);
        transparentShader.setMat4("view", view);
        transparentShader.setVec3("viewPosition", programState->camera.Position);
        transparentShader.setFloat("material.shininess", 32.0f);
        // directional light
        transparentShader.setVec3("dirLight.direction", programState->dirLightDir);
        transparentShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        transparentShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        transparentShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));

        transparentShader.setVec3("pointLights[0].position", glm::vec3(-0.8f ,0.05f, 2.7f));
        transparentShader.setVec3("pointLights[0].ambient", pointLight.ambient);
        transparentShader.setVec3("pointLights[0].diffuse", pointLight.diffuse);
        transparentShader.setVec3("pointLights[0].specular", pointLight.specular);
        transparentShader.setFloat("pointLights[0].constant", pointLight.constant);
        transparentShader.setFloat("pointLights[0].linear", pointLight.linear);
        transparentShader.setFloat("pointLights[0].quadratic", pointLight.quadratic);

        transparentShader.setVec3("pointLights[1].position", glm::vec3(-1.2f ,0.3f, -0.05f));
        transparentShader.setVec3("pointLights[1].ambient", pointLight.ambient);
        transparentShader.setVec3("pointLights[1].diffuse", pointLight.diffuse);
        transparentShader.setVec3("pointLights[1].specular", pointLight.specular);
        transparentShader.setFloat("pointLights[1].constant", pointLight.constant);
        transparentShader.setFloat("pointLights[1].linear", pointLight.linear);
        transparentShader.setFloat("pointLights[1].quadratic", pointLight.quadratic);
        // spotLight
        if (spotlightOn) {
            transparentShader.setVec3("spotLight.position", programState->camera.Position);
            transparentShader.setVec3("spotLight.direction", programState->camera.Front);
            transparentShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            transparentShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            transparentShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
            transparentShader.setFloat("spotLight.constant", 1.0f);
            transparentShader.setFloat("spotLight.linear", 0.09);
            transparentShader.setFloat("spotLight.quadratic", 0.032);
            transparentShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
            transparentShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        }else{
            transparentShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            transparentShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }
        //_____________________________________________________________________________________________________________
        //render skull2
        glm::mat4 modelSkull1 = modelSkull;
        modelSkull1 = glm::translate(modelSkull1,glm::vec3(-235.55f,165.8f+sin(time)*10.0f,-16.5f));
        modelSkull1 = glm::scale(modelSkull1, glm::vec3(1.7f));
        modelSkull1 = glm::rotate(modelSkull1,glm::radians(11.05f), glm::vec3(0.0f ,0.0f, 1.0f));
        transparentShader.setMat4("model", modelSkull1);
        skullModel.Draw(ourShader);
        //___________________________________________________________________________________________
        // draw skybox
        //___________________________________________________________________________________________
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // blur bright fragments with two-pass Gaussian Blur
        // _____________________________________________________________________________________
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 5;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        //____________________________________________________________________________________________________
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderBloomFinal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderBloomFinal.setInt("bloom", bloom);
        shaderBloomFinal.setFloat("exposure", exposure);
        renderQuad();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // _______________________________________________________________________________________
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // _______________________________________________________________________________________
    glfwTerminate();
    return 0;
}
// renderQuad() renders a 1x1 XY quad in NDC
// __________________________________________________________________________________________
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
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
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

bool circleColision(glm::vec3 c,glm::vec3 x,float r){
    float res;
    res = pow(x.x-c.x,2.0f)+pow(x.z-c.z,2.0f) - pow(r,2.0f);
    if(res >= 0)
        return true;
    else return false;
}

// process all input
// _______________________________________________________________________________________________
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}
void processInput1(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_N)==GLFW_PRESS){
        bg_changen = !bg_changen;
        //Changing background from day to night and the oposite
        if(bg_changen){
            programState->dirLightAmbDiffSpec= glm::vec3(0.01f,0.16f,0.03f);

            programState->faces =
                    {
                            FileSystem::getPath("resources/textures/skyboxn/right.jpg"),
                            FileSystem::getPath("resources/textures/skyboxn/left.jpg"),

                            FileSystem::getPath("resources/textures/skyboxn/top.jpg"),
                            FileSystem::getPath("resources/textures/skyboxn/bottom.jpg"),
                            FileSystem::getPath("resources/textures/skyboxn/front.jpg"),
                            FileSystem::getPath("resources/textures/skyboxn/back.jpg"),
                    };
            programState->cubemapTexture = loadCubemap(programState->faces);
        }else {
            programState->dirLightAmbDiffSpec= glm::vec3(0.25f,0.45f,0.6f);
            programState->faces = {
                    FileSystem::getPath("resources/textures/skybox1/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/back.jpg"),
            };
            programState->cubemapTexture = loadCubemap(programState->faces);
        }
    }

    //Colision - this don't allow us to go beyond the border of the land
        if (!(circleColision(glm::vec3(3.34f,0.0f,1.8f),programState->camera.Position,12.9f))) {
            lastCamPosition = programState->camera.Position;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                programState->camera.ProcessKeyboard(RIGHT, deltaTime);
        }else {
            programState->camera.Position = lastCamPosition;
        }


}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// _______________________________________________________________________________________________
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// _____________________________________________________________________________________________
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

//This is the comand window
void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        static float f = 0.0f;
        ImGui::Begin("Settings");
        ImGui::Text("Point light settings:");

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.005, 0.0001, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.005, 0.0001, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.005, 0.0001, 1.0);


        ImGui::Text("DirLight settings");
        ImGui::DragFloat3("Direction light direction", (float*)&programState->dirLightDir, 0.05, -20.0, 20.0);

        ImGui::Text("Ambient    Diffuse    Specular");
        ImGui::DragFloat3("Direction light settings", (float*)&programState->dirLightAmbDiffSpec, 0.05, 0.001, 1.0);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS){
        if(spotlightOn){
            spotlightOn = false;
        }else{
            spotlightOn = true;
        }
    }
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS){
        if(freeMove){
            freeMove = false;
        }else{
            freeMove = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (exposure > 0.0f)
            exposure -= 0.01f;
        else
            exposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        exposure += 0.01f;
    }

}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
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

unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
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