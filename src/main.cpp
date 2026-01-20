/**
 * @file main.cpp
 * @brief 3D Classroom Simulation Engine (OpenGL 3.3 Core)
 *
 * @details A custom rendering engine demonstrating advanced graphics techniques:
 * - **Multipass Rendering:** Dynamic shadow mapping using Depth RTT (Render-To-Texture).
 * - **Texture Arrays:** Optimized shadow storage for 9 simultaneous light sources.
 * - **Tangent Space Normal Mapping:** High-fidelity surface detail simulation.
 * - **Hardware Instancing:** GPU-accelerated rendering for repeated geometry (benches, fans).
 * - **Transparency:** Alpha blending with manual depth sorting for glass materials.
 *
 * @author Moksham
 * @date January 2026
 * @copyright MIT License
 */

// =================================================================
// 1. INCLUDES & DEPENDENCIES
// =================================================================

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>

// OpenGL Extension Wrangler
#include <GL/glew.h>

// Window Management
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Project Utilities (Credit: opengl-tutorial.org)
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/tangentspace.hpp>

// =================================================================
// 2. CONFIGURATION & CONSTANTS
// =================================================================

constexpr int WINDOW_WIDTH = 1536;
constexpr int WINDOW_HEIGHT = 1152;
const char* WINDOW_TITLE = "OpenGL Classroom Simulation - Final";

// Shadow Mapping Settings
// We use 1024x1024 resolution for the depth map.
// NUM_LIGHTS = 9 corresponds to the 3x3 grid of lights in the ceiling.
constexpr int SHADOW_WIDTH = 1024;
constexpr int SHADOW_HEIGHT = 1024;
constexpr int NUM_LIGHTS = 9;

// Global Window Handle (Required for external input controls)
GLFWwindow* window;

// =================================================================
// 3. DATA STRUCTURES
// =================================================================

/**
 * @brief Encapsulates a 3D Model's geometry and material data.
 * @note Implements RAII pattern via the Dispose() method for memory cleanup.
 */
struct Mesh {
    // -- Geometry Buffers (VBOs) --
    GLuint vertexBuffer = 0;    // Layout 0: Position
    GLuint uvBuffer = 0;        // Layout 1: UV Coords
    GLuint normalBuffer = 0;    // Layout 2: Normals
    GLuint elementBuffer = 0;   // EBO: Indices
    
    // -- Normal Mapping (Tangent Space) --
    GLuint tangentBuffer = 0;   // Layout 3
    GLuint bitangentBuffer = 0; // Layout 4

    // -- Materials --
    GLuint textureID = 0;         // Diffuse Map
    GLuint normalTextureID = 0;   // Normal Map
    GLuint specularTextureID = 0; // Specular Map

    unsigned int indexCount = 0;
    bool hasNormalMap = false;

    // -- Hardware Instancing --
    // Stores transformation matrices for every instance of this object.
    std::vector<glm::mat4> modelMatrices;

    void addInstance(const glm::mat4& matrix) {
        modelMatrices.push_back(matrix);
    }

    // Release GPU memory
    void Dispose() {
        if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
        if (uvBuffer) glDeleteBuffers(1, &uvBuffer);
        if (normalBuffer) glDeleteBuffers(1, &normalBuffer);
        if (elementBuffer) glDeleteBuffers(1, &elementBuffer);
        if (textureID) glDeleteTextures(1, &textureID);

        if (hasNormalMap) {
            glDeleteBuffers(1, &tangentBuffer);
            glDeleteBuffers(1, &bitangentBuffer);
            glDeleteTextures(1, &normalTextureID);
            glDeleteTextures(1, &specularTextureID);
        }
    }
};

/**
 * @brief Caches Shader Uniform Locations to avoid string lookups during the render loop.
 */
struct RenderUniforms {
    GLuint MatrixID, ModelMatrixID, TextureID;
    GLuint NormalSamplerID, SpecularSamplerID, SmudgeSamplerID;
    GLuint bUseNormalMapID, bUseSpecularMapID;
    GLuint AlphaID, UnlitID, bIsGlassID;
};

// =================================================================
// 4. ENGINE CLASS DEFINITION
// =================================================================

class ClassroomSimulator {
public:
    ClassroomSimulator();
    ~ClassroomSimulator();

    /**
     * @brief Main entry point. Initializes subsystems and starts the game loop.
     */
    void Run();

private:
    // --- System Handles ---
    GLuint VertexArrayID = 0;
    GLuint FramebufferName = 0;
    GLuint depthTextureArray = 0;

    // --- Shader Systems ---
    GLuint programID = 0;       // Main lighting shader
    GLuint depthProgramID = 0;  // Shadow generation shader
    GLuint depthMatrixID = 0;
    RenderUniforms uniforms;

    // --- Shader Handles ---
    GLuint ViewMatrixID;
    GLuint DepthBiasMatricesID;
    GLuint ShadowMapArrayID;
    GLuint ShadingModelID;
    GLuint ClassroomLightPositionsID;

    // --- Scene Assets ---
    // Meshes grouped by type for optimized rendering
    Mesh bench, door, switchObj, exhaust, clock, pipe, projector, screen;
    Mesh floorMesh, fan, greenboard, podium, table, lightPanel, grid;
    Mesh windowMesh, wallFan, glass;
    Mesh wall, ceiling;

    // --- Render Lists ---
    // Pointers to meshes, sorted by shader requirements
    std::vector<Mesh*> opaqueMeshes;
    std::vector<Mesh*> normalMapMeshes;
    std::vector<Mesh*> transparentMeshes;

    // --- Lighting State ---
    glm::vec3 classroomLightPositions_worldspace[NUM_LIGHTS];

    // --- Internal Methods ---
    bool InitSystem();
    bool InitShadowFramebuffer();
    void InitShaders();
    void LoadScene();
    void MainLoop();
    void Cleanup();

    // Utilities
    Mesh LoadStandardMesh(const char* objPath, const char* ddsPath);
    Mesh LoadNormalMapMesh(const char* objPath, const char* diffPath, const char* normPath, const char* specPath);
    void AddObject(Mesh& mesh, glm::vec3 pos, float rotDeg = 0.0f, glm::vec3 rotAxis = glm::vec3(0,1,0), glm::vec3 scale = glm::vec3(1.0f));
    
    void DrawMesh(const Mesh& mesh, const RenderUniforms& uniforms, const glm::mat4& View, const glm::mat4& Projection, float alpha = 1.0f);
    void DrawMeshShadow(const Mesh& mesh, GLuint depthMatrixID, const glm::mat4& dProj, const glm::mat4& dView);
};

// =================================================================
// 5. MAIN ENTRY POINT
// =================================================================

int main(void)
{
    // Allocate on heap to prevent stack overflow
    ClassroomSimulator* app = new ClassroomSimulator();
    app->Run();
    delete app;
    return 0;
}

// =================================================================
// 6. CLASS IMPLEMENTATION
// =================================================================

ClassroomSimulator::ClassroomSimulator() {
    // Constructor handles zero-initialization via member initializers
}

ClassroomSimulator::~ClassroomSimulator() {
    Cleanup();
}

void ClassroomSimulator::Run() {
    // 1. Initialize Window & OpenGL Context
    if (!InitSystem()) return;

    // 2. Compile Shaders & Link Uniforms
    InitShaders();

    // 3. Create FBO for Shadow Mapping
    if (!InitShadowFramebuffer()) return;
    
    // 4. Load Models & Compose Scene
    LoadScene();
    
    // 5. Enter Infinite Render Loop
    MainLoop();
}

bool ClassroomSimulator::InitSystem() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Configure OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (window == nullptr) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return false;
    }

    // Input Mode: FPS Style (Capture Mouse)
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents();
    glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    
    // Global GL Config
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    return true;
}

bool ClassroomSimulator::InitShadowFramebuffer() {
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    // Texture Array: Stores 9 depth maps in a single texture object
    glGenTextures(1, &depthTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextureArray);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT16, 
                 SHADOW_WIDTH, SHADOW_HEIGHT, NUM_LIGHTS, 
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // PCF Filtering Parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextureArray, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error: Shadow Framebuffer is incomplete!\n");
        return false;
    }
    return true;
}

void ClassroomSimulator::InitShaders() {
    // Load GLSL Code
    depthProgramID = LoadShaders("shaders/DepthRTT.vertexshader", "shaders/DepthRTT.fragmentshader");
    depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

    programID = LoadShaders("shaders/ShadowMapping.vertexshader", "shaders/ShadowMapping.fragmentshader");
    
    // Bind Uniforms
    uniforms.MatrixID          = glGetUniformLocation(programID, "MVP");
    uniforms.ModelMatrixID     = glGetUniformLocation(programID, "M");
    uniforms.TextureID         = glGetUniformLocation(programID, "myTextureSampler");
    uniforms.bUseNormalMapID   = glGetUniformLocation(programID, "bUseNormalMap");
    uniforms.bUseSpecularMapID = glGetUniformLocation(programID, "bUseSpecularMap");
    uniforms.NormalSamplerID   = glGetUniformLocation(programID, "NormalTextureSampler");
    uniforms.SpecularSamplerID = glGetUniformLocation(programID, "SpecularTextureSampler");
    uniforms.AlphaID           = glGetUniformLocation(programID, "fragmentAlpha");
    uniforms.UnlitID           = glGetUniformLocation(programID, "bIsUnlit");
    uniforms.bIsGlassID        = glGetUniformLocation(programID, "bIsGlass");
    uniforms.SmudgeSamplerID   = glGetUniformLocation(programID, "SmudgeSampler");

    ViewMatrixID              = glGetUniformLocation(programID, "V");
    DepthBiasMatricesID       = glGetUniformLocation(programID, "DepthBiasMVPs");
    ShadowMapArrayID          = glGetUniformLocation(programID, "shadowMapArray");
    ShadingModelID            = glGetUniformLocation(programID, "uShadingModel");
    ClassroomLightPositionsID = glGetUniformLocation(programID, "ClassroomLightPositions_cameraspace");
}

void ClassroomSimulator::LoadScene() {
    printf("Loading Assets (This may take a moment)...\n");

    auto Mod = [](const char* name) { return std::string("assets/models/") + name; };
    auto Tex = [](const char* name) { return std::string("assets/textures/") + name; };

    // --- 1. Load Opaque Meshes ---
    bench      = LoadStandardMesh(Mod("bench.obj").c_str(),      Tex("bench.dds").c_str());
    door       = LoadStandardMesh(Mod("door.obj").c_str(),       Tex("door.dds").c_str());
    switchObj  = LoadStandardMesh(Mod("switch.obj").c_str(),     Tex("switch.dds").c_str());
    exhaust    = LoadStandardMesh(Mod("exhaust.obj").c_str(),    Tex("projector.dds").c_str());
    clock      = LoadStandardMesh(Mod("clock.obj").c_str(),      Tex("clock.dds").c_str());
    pipe       = LoadStandardMesh(Mod("pipe.obj").c_str(),       Tex("pipe.dds").c_str());
    projector  = LoadStandardMesh(Mod("projector.obj").c_str(),  Tex("projector.dds").c_str());
    screen     = LoadStandardMesh(Mod("screen.obj").c_str(),     Tex("screen.dds").c_str());
    floorMesh  = LoadStandardMesh(Mod("floor.obj").c_str(),      Tex("floor.dds").c_str());
    fan        = LoadStandardMesh(Mod("fan.obj").c_str(),        Tex("fan.dds").c_str());
    greenboard = LoadStandardMesh(Mod("greenboard.obj").c_str(), Tex("greenboard.dds").c_str());
    podium     = LoadStandardMesh(Mod("podium.obj").c_str(),     Tex("podium.dds").c_str());
    table      = LoadStandardMesh(Mod("table.obj").c_str(),      Tex("table.dds").c_str());
    lightPanel = LoadStandardMesh(Mod("lightpanel.obj").c_str(), Tex("lightpanel.dds").c_str());
    grid       = LoadStandardMesh(Mod("grid.obj").c_str(),       Tex("grid.dds").c_str());
    windowMesh = LoadStandardMesh(Mod("window.obj").c_str(),     Tex("window.dds").c_str());
    wallFan    = LoadStandardMesh(Mod("wallfan.obj").c_str(),    Tex("wallfan.dds").c_str());

    // --- 2. Load Transparent Meshes ---
    glass      = LoadStandardMesh(Mod("glass.obj").c_str(),     Tex("glass.dds").c_str());

    // --- 3. Load Normal Mapped Meshes ---
    // These require Tangent/Bitangent calculation for the shader
    wall    = LoadNormalMapMesh(Mod("walls.obj").c_str(),   Tex("walls.dds").c_str(),   Tex("normal.bmp").c_str(), Tex("specular.dds").c_str());
    ceiling = LoadNormalMapMesh(Mod("ceiling.obj").c_str(), Tex("ceiling.dds").c_str(),  Tex("normal.bmp").c_str(), Tex("specular.dds").c_str());

    // --- 4. Populate Render Buckets ---
    // Grouping pointers allows iterating through types in the render loop
    opaqueMeshes = { 
        &bench, &door, &switchObj, &exhaust, &clock, &pipe, &projector, 
        &screen, &floorMesh, &fan, &greenboard, &podium, &table, &windowMesh, &wallFan 
    };
    normalMapMeshes = { &wall, &ceiling, &grid };
    transparentMeshes = { &glass };

    // --- 5. Scene Composition (Instancing) ---
    
    // Benches (5x5 Grid)
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (i == 0 && (j == 3 || j == 4)) continue; // Leave Aisle
            AddObject(bench, glm::vec3(-16.0f + i * 9.50f, 0.5f, -40.0f + j * 20.0f), 90.0f, glm::vec3(0,1,0));
        }
    }
    
    // Ceiling Fans (2x3 Grid)
    glm::vec3 fanStart = glm::vec3(-32.2 + 19.32f, 32.975f, -48.3 + 22.54f);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            AddObject(fan, fanStart + glm::vec3(i * 25.76f, 0.0f, j * 25.76f));
        }
    }

    // Static Geometry
    AddObject(floorMesh, glm::vec3(0.0f));
    AddObject(ceiling,   glm::vec3(0.0f, 38.1f, 0.0f));
    AddObject(wall,      glm::vec3(-32.7f, 19.06f, 0.0f));
    AddObject(door,      glm::vec3(-31.7f, 12.5f, 48.8f));
    
    // Ceiling Grid (Rotated)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0, 38.8, 0));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(180.0f), glm::vec3(1, 0, 0));
        grid.addInstance(m);
    }

    // Details
    for (int i = 0; i < 2; i++) {
        AddObject(greenboard, glm::vec3(-32.2f, 18.6f, i * 36 * 0.8f - 27.7f + 3.6f), 0.0f, glm::vec3(0,1,0), glm::vec3(1, 1, 0.8));
        AddObject(switchObj, glm::vec3(-10.2f + i * 28.2f, 14.6f, 48.3f), 180.0f, glm::vec3(0,1,0), glm::vec3(0.7f));
        AddObject(exhaust, glm::vec3(14.23f - i * 21.46f, 34.1f, 48.8f), 0.0f, glm::vec3(0,1,0), glm::vec3(0.857f));
    }

    AddObject(podium,    glm::vec3(-20.0f, 0.5f, 28.0f), 290.0f);
    AddObject(table,     glm::vec3(-9.0f, 0.5f, 13.20f), 90.0f);
    AddObject(projector, glm::vec3(6.44f, 29.75f, -3.22f), 180.0f);
    AddObject(screen,    glm::vec3(-31.5f, 30.0f, -9.66f), 0.0f, glm::vec3(0,1,0), glm::vec3(1, 1.2f, 1.5f));
    AddObject(clock,     glm::vec3(7.60f, 28.0f, -48.0f), 90.0f, glm::vec3(1,0,0));
    AddObject(pipe,      glm::vec3(-32.2f, 5.0f, -9.0f), 90.0f);
    AddObject(wallFan,   glm::vec3(-14.0f, 25.0f, 48.3f), 180.0f, glm::vec3(0,1,0), glm::vec3(0.5f));

    // Windows & Glass
    for (int i = 0; i < 8; i++) {
        glm::vec3 pos(32.70f, 34.1f, -42.26f + 12.075f * i);
        AddObject(windowMesh, pos, 90.0f);
        AddObject(glass,      pos, 90.0f, glm::vec3(0,1,0), glm::vec3(1.0f, 1.0f, .250f));
    }
    for (int i = 0; i < 6; i++) {
        glm::vec3 pos(26.83f - 10.73f * i, 34.1f, 48.8f);
        AddObject(windowMesh, pos, 0.0f, glm::vec3(0,1,0), glm::vec3(0.888f, 1.0f, 1.0f));
        AddObject(glass,      pos, 0.0f, glm::vec3(0,1,0), glm::vec3(1.0f, 1.0f, 0.25f));
    }

    // Lights
    int lightIdx = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float cx = -22.54f + i * 25.76f;
            float cz = -25.76f + j * 25.76f;
            classroomLightPositions_worldspace[lightIdx++] = glm::vec3(cx, 38.6f, cz);
            // Add visual representation of light
            AddObject(lightPanel, glm::vec3(cx, 37.675f, cz), 0.0f, glm::vec3(0,1,0), glm::vec3(6.44f, 0.2f, 6.44f));
        }
    }
}

void ClassroomSimulator::MainLoop() {
    int shadingMode = 0;
    bool gKeyPressed = false;
    std::vector<glm::mat4> allDepthBiasMVPs;
    int lightIdx = 0;

    printf("Initialization Complete. Starting Loop...\n");

    do {
        // --- Input Handling ---
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            if (!gKeyPressed) {
                shadingMode = !shadingMode;
                gKeyPressed = true;
                printf("Shading Mode: %s\n", shadingMode == 1 ? "Gouraud" : "Phong");
            }
        } else { gKeyPressed = false; }

        // ============================================================
        // PASS 1: SHADOW MAPPING (Depth Generation)
        // ============================================================
        // Renders the scene from the perspective of each light source.
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK); 
        glUseProgram(depthProgramID);
        allDepthBiasMVPs.clear();

        lightIdx = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                // Target the specific layer in the Texture Array
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextureArray, 0, lightIdx);
                glClear(GL_DEPTH_BUFFER_BIT);

                // Compute Light View/Projection
                glm::vec3 lightPos = classroomLightPositions_worldspace[lightIdx];
                glm::mat4 depthView = glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
                glm::mat4 depthProj = glm::perspective(glm::radians(120.0f), 1.5f, 5.0f, 1000.0f);
                
                // Draw Shadow Casters
                for (Mesh* mesh : opaqueMeshes) DrawMeshShadow(*mesh, depthMatrixID, depthProj, depthView);
                for (Mesh* mesh : normalMapMeshes) DrawMeshShadow(*mesh, depthMatrixID, depthProj, depthView);
                
                // Calculate Depth Bias Matrix to map [-1,1] to [0,1]
                glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
                allDepthBiasMVPs.push_back(biasMatrix * depthProj * depthView);
                lightIdx++;
            }
        }

        // ============================================================
        // PASS 2: MAIN RENDERING (Lighting)
        // ============================================================
        
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Render to Screen
        glViewport(0, 0, w, h);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programID);
        glUniform1i(ShadingModelID, shadingMode);

        // Update Camera Matrices (View/Projection)
        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();

        // Pass Light Data to Shader
        glm::vec3 lightPosCameraSpace[NUM_LIGHTS];
        for (int i = 0; i < NUM_LIGHTS; i++) {
            lightPosCameraSpace[i] = glm::vec3(ViewMatrix * glm::vec4(classroomLightPositions_worldspace[i], 1.0));
        }
        glUniform3fv(ClassroomLightPositionsID, NUM_LIGHTS, &lightPosCameraSpace[0][0]);
        glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
        glUniformMatrix4fv(DepthBiasMatricesID, NUM_LIGHTS, GL_FALSE, &allDepthBiasMVPs[0][0][0]);

        // Bind Shadow Maps
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextureArray);
        glUniform1i(ShadowMapArrayID, 1);
        glUniform1i(uniforms.UnlitID, 0);

        // 1. Draw Opaque
        for (Mesh* mesh : opaqueMeshes) DrawMesh(*mesh, uniforms, ViewMatrix, ProjectionMatrix);
        
        // 2. Draw Normal Mapped
        for (Mesh* mesh : normalMapMeshes) DrawMesh(*mesh, uniforms, ViewMatrix, ProjectionMatrix);

        // 3. Draw Unlit (Light Panels)
        glUniform1i(uniforms.UnlitID, 1);
        DrawMesh(lightPanel, uniforms, ViewMatrix, ProjectionMatrix);
        glUniform1i(uniforms.UnlitID, 0);

        // 4. Draw Transparent (Sorted Last)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // Read-only depth buffer
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, glass.textureID);
        glUniform1i(uniforms.SmudgeSamplerID, 3);
        glUniform1i(uniforms.bIsGlassID, 1);

        for (Mesh* mesh : transparentMeshes) DrawMesh(*mesh, uniforms, ViewMatrix, ProjectionMatrix, 0.25f);

        // Reset State
        glUniform1i(uniforms.bIsGlassID, 0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
}

void ClassroomSimulator::Cleanup() {
    // Release Meshes
    // We iterate a temporary combined list to ensure we don't miss anything.
    std::vector<Mesh*> allMeshes = opaqueMeshes;
    allMeshes.insert(allMeshes.end(), normalMapMeshes.begin(), normalMapMeshes.end());
    allMeshes.insert(allMeshes.end(), transparentMeshes.begin(), transparentMeshes.end());
    allMeshes.push_back(&lightPanel);

    for(Mesh* m : allMeshes) {
        if (m) m->Dispose();
    }

    // Release GL Objects
    if (FramebufferName) glDeleteFramebuffers(1, &FramebufferName);
    if (depthTextureArray) glDeleteTextures(1, &depthTextureArray);
    if (VertexArrayID) glDeleteVertexArrays(1, &VertexArrayID);
    if (programID) glDeleteProgram(programID);
    if (depthProgramID) glDeleteProgram(depthProgramID);
    
    glfwTerminate();
    printf("Shutdown Complete.\n");
}

// =================================================================
// HELPERS
// =================================================================

void ClassroomSimulator::AddObject(Mesh& mesh, glm::vec3 pos, float rotDeg, glm::vec3 rotAxis, glm::vec3 scale) 
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    if (rotDeg != 0.0f) model = glm::rotate(model, glm::radians(rotDeg), rotAxis);
    if (scale != glm::vec3(1.0f)) model = glm::scale(model, scale);
    mesh.addInstance(model);
}

Mesh ClassroomSimulator::LoadStandardMesh(const char* objPath, const char* ddsPath) {
    Mesh mesh;
    mesh.textureID = loadDDS(ddsPath);
    
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    loadOBJ(objPath, vertices, uvs, normals);

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> i_vertices, i_normals;
    std::vector<glm::vec2> i_uvs;
    indexVBO(vertices, uvs, normals, indices, i_vertices, i_uvs, i_normals);

    mesh.indexCount = indices.size();
    mesh.hasNormalMap = false;

    glGenBuffers(1, &mesh.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_vertices.size() * sizeof(glm::vec3), &i_vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_uvs.size() * sizeof(glm::vec2), &i_uvs[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_normals.size() * sizeof(glm::vec3), &i_normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

    return mesh;
}

Mesh ClassroomSimulator::LoadNormalMapMesh(const char* objPath, const char* diffPath, const char* normPath, const char* specPath) {
    Mesh mesh;
    mesh.textureID = loadDDS(diffPath);
    mesh.normalTextureID = loadBMP_custom(normPath);
    mesh.specularTextureID = loadDDS(specPath);
    mesh.hasNormalMap = true;

    std::vector<glm::vec3> vertices, normals;
    std::vector<glm::vec2> uvs;
    loadOBJ(objPath, vertices, uvs, normals);

    std::vector<glm::vec3> tangents, bitangents;
    computeTangentBasis(vertices, uvs, normals, tangents, bitangents);

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> i_vertices, i_normals, i_tangents, i_bitangents;
    std::vector<glm::vec2> i_uvs;

    indexVBO_TBN(vertices, uvs, normals, tangents, bitangents, indices, 
                 i_vertices, i_uvs, i_normals, i_tangents, i_bitangents);

    mesh.indexCount = indices.size();

    glGenBuffers(1, &mesh.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_vertices.size() * sizeof(glm::vec3), &i_vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_uvs.size() * sizeof(glm::vec2), &i_uvs[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_normals.size() * sizeof(glm::vec3), &i_normals[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.tangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.tangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_tangents.size() * sizeof(glm::vec3), &i_tangents[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.bitangentBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.bitangentBuffer);
    glBufferData(GL_ARRAY_BUFFER, i_bitangents.size() * sizeof(glm::vec3), &i_bitangents[0], GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.elementBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

    return mesh;
}

void ClassroomSimulator::DrawMesh(const Mesh& mesh, const RenderUniforms& uniforms, const glm::mat4& View, const glm::mat4& Projection, float alpha) 
{
    // Bind Material Textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.textureID);
    glUniform1i(uniforms.TextureID, 0);

    if (mesh.hasNormalMap) {
        glUniform1i(uniforms.bUseNormalMapID, 1);
        glUniform1i(uniforms.bUseSpecularMapID, 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mesh.normalTextureID);
        glUniform1i(uniforms.NormalSamplerID, 2);
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mesh.specularTextureID);
        glUniform1i(uniforms.SpecularSamplerID, 3);
    } else {
        glUniform1i(uniforms.bUseNormalMapID, 0);
        glUniform1i(uniforms.bUseSpecularMapID, 0);
    }

    // Bind Geometry Attributes
    glEnableVertexAttribArray(0); glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1); glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffer);     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2); glBindBuffer(GL_ARRAY_BUFFER, mesh.normalBuffer); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    if (mesh.hasNormalMap) {
        glEnableVertexAttribArray(3); glBindBuffer(GL_ARRAY_BUFFER, mesh.tangentBuffer);   glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(4); glBindBuffer(GL_ARRAY_BUFFER, mesh.bitangentBuffer); glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    } else {
        glDisableVertexAttribArray(3);
        glDisableVertexAttribArray(4);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementBuffer);
    glUniform1f(uniforms.AlphaID, alpha);

    // Instanced Draw Loop
    for (const auto& modelMatrix : mesh.modelMatrices) {
        glm::mat4 MVP = Projection * View * modelMatrix;
        glUniformMatrix4fv(uniforms.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(uniforms.ModelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, nullptr);
    }

    glDisableVertexAttribArray(0); glDisableVertexAttribArray(1); glDisableVertexAttribArray(2);
}

void ClassroomSimulator::DrawMeshShadow(const Mesh& mesh, GLuint depthMatrixID, const glm::mat4& dProj, const glm::mat4& dView) 
{
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementBuffer);

    for (const auto& modelMatrix : mesh.modelMatrices) {
        glm::mat4 depthMVP = dProj * dView * modelMatrix;
        glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP[0][0]);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_SHORT, nullptr);
    }
    glDisableVertexAttribArray(0);
}
