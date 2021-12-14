

#ifdef _WIN32
extern "C" _declspec(dllexport)
unsigned int NvOptimusEnablement = 0x00000001;
#endif

#include <GL/glew.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>

#include <labhelper.h>
#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;

#include <Model.h>
#include "hdr.h"
#include "fbo.h"
#include "heightfield.h"


using std::min;
using std::max;

///////////////////////////////////////////////////////////////////////////////
// Various globals
///////////////////////////////////////////////////////////////////////////////
SDL_Window* g_window = nullptr;
float currentTime = 0.0f;
float previousTime = 0.0f;
float deltaTime = 0.0f;
bool showUI = true;
int windowWidth, windowHeight;

// Mouse input
ivec2 g_prevMouseCoords = {-1, -1};
bool g_isMouseDragging = false;
bool g_isMouseRightDragging = false;

///////////////////////////////////////////////////////////////////////////////
// Shader programs
///////////////////////////////////////////////////////////////////////////////
GLuint shaderProgram;       // Shader for rendering the final image
GLuint simpleShaderProgram; // Shader used to draw the shadow map
GLuint backgroundProgram;
GLuint heightfieldProgram;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
float environment_multiplier = 1.5f;
GLuint environmentMap, irradianceMap, reflectionMap;
const std::string envmap_base_name = "001";

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
vec3 lightPosition;
float lightRotation = 0.f;
vec3 point_light_color = vec3(1.f, 1.f, 1.f);
bool lightManualOnly = false;

float point_light_intensity_multiplier = 10000.0f;

///////////////////////////////////////////////////////////////////////////////
// Shadow map
///////////////////////////////////////////////////////////////////////////////
enum ClampMode {
    Edge = 1,
    Border = 2
};

FboInfo shadowMapFB;
int shadowMapResolution = 1024;
int shadowMapClampMode = ClampMode::Edge;
bool shadowMapClampBorderShadowed = false;
bool usePolygonOffset = true;
bool useSoftFalloff = false;
bool useHardwarePCF = false;
float polygonOffset_factor = 1.f;
float polygonOffset_units = 1.0f;

///////////////////////////////////////////////////////////////////////////////
// Camera parameters.
///////////////////////////////////////////////////////////////////////////////
vec3 cameraPosition(-70.0f, 50.0f, 70.0f);
vec3 cameraDirection = normalize(vec3(0.0f) - cameraPosition);
float cameraSpeed = 30.f;

vec3 worldUp(0.0f, 1.0f, 0.0f);
float rotation_speed = 15.f;

///////////////////////////////////////////////////////////////////////////////
// Models
///////////////////////////////////////////////////////////////////////////////
labhelper::Model* sphereModel = nullptr;

HeightField terrain;
bool onlyTrianglesMesh = false;
int tessellation = 256;
float meshHeightIntensity = 50.f;
float meshDensityIntensity = 300.f;
float terrainSize = 100.f;
float randomSeed = 100.;

mat4 roomModelMatrix;
mat4 landingPadModelMatrix;
mat4 fighterModelMatrix;

void loadShaders(bool is_reload) {
    GLuint shader = labhelper::loadShaderProgram("../project/simple.vert", "../project/simple.frag",
                                                 is_reload);
    if (shader != 0) {
        simpleShaderProgram = shader;
    }
    shader = labhelper::loadShaderProgram("../project/background.vert", "../project/background.frag",
                                          is_reload);
    if (shader != 0) {
        backgroundProgram = shader;
    }
    shader = labhelper::loadShaderProgram("../project/shading.vert", "../project/shading.frag", is_reload);
    if (shader != 0) {
        shaderProgram = shader;
    }
    shader = labhelper::loadShaderProgram("../project/heightfield.vert", "../project/heightfield.frag", is_reload);
    if (shader != 0) {
        heightfieldProgram = shader;
    }
}

void initGL() {
    ///////////////////////////////////////////////////////////////////////
    //		Load Shaders
    ///////////////////////////////////////////////////////////////////////
    heightfieldProgram = labhelper::loadShaderProgram("../project/heightfield.vert", "../project/heightfield.frag");
    backgroundProgram = labhelper::loadShaderProgram("../project/background.vert",
                                                     "../project/background.frag");
    shaderProgram = labhelper::loadShaderProgram("../project/shading.vert", "../project/shading.frag");
    simpleShaderProgram = labhelper::loadShaderProgram("../project/simple.vert", "../project/simple.frag");

    ///////////////////////////////////////////////////////////////////////
    // Load models and set up model matrices
    ///////////////////////////////////////////////////////////////////////
    sphereModel = labhelper::loadModelFromOBJ("../scenes/sphere.obj");

    roomModelMatrix = mat4(1.0f);
    fighterModelMatrix = translate(15.0f * worldUp);
    landingPadModelMatrix = mat4(1.0f);

    ///////////////////////////////////////////////////////////////////////
    // Load environment map
    ///////////////////////////////////////////////////////////////////////
    const int roughnesses = 8;
    std::vector<std::string> filenames;
    filenames.reserve(roughnesses);
    for (int i = 0; i < roughnesses; i++) {
        filenames.push_back("../scenes/envmaps/" + envmap_base_name + "_dl_" + std::to_string(i) + ".hdr");
    }

    reflectionMap = labhelper::loadHdrMipmapTexture(filenames);
    environmentMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + ".hdr");
    irradianceMap = labhelper::loadHdrTexture("../scenes/envmaps/" + envmap_base_name + "_irradiance.hdr");

    shadowMapFB.resize(shadowMapResolution, shadowMapResolution);
    glBindTexture(GL_TEXTURE_2D, shadowMapFB.depthBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    glEnable(GL_DEPTH_TEST); // enable Z-buffering
    glEnable(GL_CULL_FACE);  // enables backface culling

    terrain.generateMesh(tessellation);
}

void debugDrawLight(const glm::mat4& viewMatrix,
                    const glm::mat4& projectionMatrix,
                    const glm::vec3& worldSpaceLightPos) {
    mat4 modelMatrix = glm::translate(worldSpaceLightPos);
    glUseProgram(shaderProgram);
    labhelper::setUniformSlow(shaderProgram, "modelViewProjectionMatrix",
                              projectionMatrix * viewMatrix * modelMatrix);
    labhelper::render(sphereModel);
}


void drawBackground(const mat4& viewMatrix, const mat4& projectionMatrix) {
    glUseProgram(backgroundProgram);
    labhelper::setUniformSlow(backgroundProgram, "environment_multiplier", environment_multiplier);
    labhelper::setUniformSlow(backgroundProgram, "inv_PV", inverse(projectionMatrix * viewMatrix));
    labhelper::setUniformSlow(backgroundProgram, "camera_pos", cameraPosition);
    labhelper::drawFullScreenQuad();
}

void drawMesh(GLuint currentShaderProgram,
              const mat4& viewMatrix,
              const mat4& projectionMatrix,
              const mat4& lightViewMatrix,
              const mat4& lightProjectionMatrix) {
    glUseProgram(currentShaderProgram);

    // Light source
    vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
    labhelper::setUniformSlow(currentShaderProgram, "point_light_color", point_light_color);
    labhelper::setUniformSlow(currentShaderProgram, "point_light_intensity_multiplier",
                              point_light_intensity_multiplier);
    labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
    labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightDir",
                              normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));
    mat4 lightMatrix = translate(vec3(0.5f))
                       * scale(vec3(0.5f))
                       * lightProjectionMatrix
                       * lightViewMatrix
                       * inverse(viewMatrix);
    labhelper::setUniformSlow(currentShaderProgram, "lightMatrix", lightMatrix);

    labhelper::setUniformSlow(currentShaderProgram, "environment_multiplier", environment_multiplier);

    mat4 modelMatrix = rotate(radians(-45.f), vec3(0., 1., 0.))
        * scale(mat4(1.f), vec3(terrainSize, 25.f, terrainSize));

    labhelper::setUniformSlow(currentShaderProgram, "seed", vec2(randomSeed, randomSeed / 2));

    labhelper::setUniformSlow(currentShaderProgram, "modelViewMatrix", viewMatrix * modelMatrix);
    labhelper::setUniformSlow(currentShaderProgram, "normalMatrix",
                              inverse(transpose(viewMatrix * modelMatrix)));

    labhelper::setUniformSlow(currentShaderProgram, "viewInverse", inverse(viewMatrix));
    labhelper::setUniformSlow(currentShaderProgram, "modelViewProjectionMatrix", projectionMatrix * viewMatrix * modelMatrix);
    labhelper::setUniformSlow(currentShaderProgram, "densityIntensity", (meshDensityIntensity * terrainSize) / 100);
    labhelper::setUniformSlow(currentShaderProgram, "heightIntensity", meshHeightIntensity / 100);

    terrain.submitTriangles(onlyTrianglesMesh);
}

void drawScene(GLuint currentShaderProgram,
               const mat4& viewMatrix,
               const mat4& projectionMatrix,
               const mat4& lightViewMatrix,
               const mat4& lightProjectionMatrix) {
    glUseProgram(currentShaderProgram);
    // Light source
    vec4 viewSpaceLightPosition = viewMatrix * vec4(lightPosition, 1.0f);
    labhelper::setUniformSlow(currentShaderProgram, "point_light_color", point_light_color);
    labhelper::setUniformSlow(currentShaderProgram, "point_light_intensity_multiplier",
                              point_light_intensity_multiplier);
    labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightPosition", vec3(viewSpaceLightPosition));
    labhelper::setUniformSlow(currentShaderProgram, "viewSpaceLightDir",
                              normalize(vec3(viewMatrix * vec4(-lightPosition, 0.0f))));
    mat4 lightMatrix = translate(vec3(0.5f))
                       * scale(vec3(0.5f))
                       * lightProjectionMatrix
                       * lightViewMatrix
                       * inverse(viewMatrix);
    labhelper::setUniformSlow(currentShaderProgram, "lightMatrix", lightMatrix);

    // Environment
    labhelper::setUniformSlow(currentShaderProgram, "environment_multiplier", environment_multiplier);

    // camera
    labhelper::setUniformSlow(currentShaderProgram, "viewInverse", inverse(viewMatrix));
}

void display() {
    ///////////////////////////////////////////////////////////////////////////
    // Check if window size has changed and resize buffers as needed
    ///////////////////////////////////////////////////////////////////////////
    {
        int w, h;
        SDL_GetWindowSize(g_window, &w, &h);
        if (w != windowWidth || h != windowHeight) {
            windowWidth = w;
            windowHeight = h;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // setup matrices
    ///////////////////////////////////////////////////////////////////////////
    mat4 projMatrix = perspective(radians(45.0f), float(windowWidth) / float(windowHeight), 5.0f, 2000.0f);
    mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);

    vec4 lightStartPosition = vec4(40.0f, 40.0f, 0.0f, 1.0f);
    float light_rotation_speed = 1.f;
    if (!lightManualOnly && !g_isMouseRightDragging) {
        lightRotation += deltaTime * light_rotation_speed;
    }
    lightPosition = vec3(rotate(lightRotation, worldUp) * lightStartPosition);
    mat4 lightViewMatrix = lookAt(lightPosition, vec3(0.0f), worldUp);
    mat4 lightProjMatrix = perspective(radians(45.0f), 1.0f, 25.0f, 100.0f);

    ///////////////////////////////////////////////////////////////////////////
    // Bind the environment map(s) to unused texture units
    ///////////////////////////////////////////////////////////////////////////
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, environmentMap);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, irradianceMap);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, reflectionMap);
    glActiveTexture(GL_TEXTURE0);

    ///////////////////////////////////////////////////////////////////////////
    // Set up shadow map parameters
    ///////////////////////////////////////////////////////////////////////////
    // >>> @task 1

    if (shadowMapFB.width != shadowMapResolution || shadowMapFB.height != shadowMapResolution) {
        shadowMapFB.resize(shadowMapResolution, shadowMapResolution);
    }

    GLint glMode = useHardwarePCF ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glMode);

    if (shadowMapClampMode == ClampMode::Edge) {
        glBindTexture(GL_TEXTURE_2D, shadowMapFB.depthBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if (shadowMapClampMode == ClampMode::Border) {
        glBindTexture(GL_TEXTURE_2D, shadowMapFB.depthBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        vec4 border(shadowMapClampBorderShadowed ? 0.f : 1.f);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border.x);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Draw Shadow Map
    ///////////////////////////////////////////////////////////////////////////

    if (usePolygonOffset) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(polygonOffset_factor, polygonOffset_units);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFB.framebufferId);
    glViewport(0, 0, shadowMapFB.width, shadowMapFB.height);
    glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawScene(simpleShaderProgram, lightViewMatrix, lightProjMatrix, lightViewMatrix, lightProjMatrix);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, shadowMapFB.depthBuffer);

    if (usePolygonOffset) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Draw from camera
    ///////////////////////////////////////////////////////////////////////////
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawBackground(viewMatrix, projMatrix);
    drawScene(shaderProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix);
    drawMesh(heightfieldProgram, viewMatrix, projMatrix, lightViewMatrix, lightProjMatrix);
    debugDrawLight(viewMatrix, projMatrix, vec3(lightPosition));
}

bool handleEvents() {
    // check events (keyboard among other)
    SDL_Event event;
    bool quitEvent = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
            quitEvent = true;
        }
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
            showUI = !showUI;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && (!showUI || !ImGui::GetIO().WantCaptureMouse)
            && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)
            && !(g_isMouseDragging || g_isMouseRightDragging)) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                g_isMouseDragging = true;
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                g_isMouseRightDragging = true;
            }
            int x;
            int y;
            SDL_GetMouseState(&x, &y);
            g_prevMouseCoords.x = x;
            g_prevMouseCoords.y = y;
        }

        if (!(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
            g_isMouseDragging = false;
        }
        if (!(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))) {
            g_isMouseRightDragging = false;
        }

        if (event.type == SDL_MOUSEMOTION && g_isMouseDragging) {
            // More info at https://wiki.libsdl.org/SDL_MouseMotionEvent
            float delta_x = (float) event.motion.x - (float) g_prevMouseCoords.x;
            float delta_y = (float) event.motion.y - (float) g_prevMouseCoords.y;
            if (g_isMouseDragging) {
                mat4 yaw = rotate(rotation_speed / 100.f * deltaTime * -delta_x, worldUp);
                mat4 pitch = rotate(rotation_speed / 100.f * deltaTime * -delta_y,
                                    normalize(cross(cameraDirection, worldUp)));
                cameraDirection = vec3(pitch * yaw * vec4(cameraDirection, 0.0f));
            } else if (g_isMouseRightDragging) {
                lightRotation += delta_x * 0.01f;
            }
            g_prevMouseCoords.x = event.motion.x;
            g_prevMouseCoords.y = event.motion.y;
        }
    }

    // check keyboard state (which keys are still pressed)
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    vec3 cameraRight = cross(cameraDirection, worldUp);

    if (state[SDL_SCANCODE_W]) {
        cameraPosition += cameraSpeed * deltaTime * cameraDirection;
    }
    if (state[SDL_SCANCODE_S]) {
        cameraPosition -= cameraSpeed * deltaTime * cameraDirection;
    }
    if (state[SDL_SCANCODE_A]) {
        cameraPosition -= cameraSpeed * deltaTime * cameraRight;
    }
    if (state[SDL_SCANCODE_D]) {
        cameraPosition += cameraSpeed * deltaTime * cameraRight;
    }
    if (state[SDL_SCANCODE_Q]) {
        cameraPosition -= cameraSpeed * deltaTime * worldUp;
    }
    if (state[SDL_SCANCODE_E]) {
        cameraPosition += cameraSpeed * deltaTime * worldUp;
    }
    return quitEvent;
}

void gui() {
    // Inform imgui of new frame
    ImGui_ImplSdlGL3_NewFrame(g_window);

    // ----------------- Set variables --------------------------
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    if (ImGui::CollapsingHeader("Terrain", "terrain_ch", true, true)) {
        ImGui::Checkbox("Mesh triangles only", &onlyTrianglesMesh);
        ImGui::SliderFloat("Mesh height intensity", &meshHeightIntensity, 10.f, 1000.f, "%.0f", 2.f);
        ImGui::SliderFloat("Mesh density intensity", &meshDensityIntensity, 100.f, 2000.f, "%.0f", 2.f);
        ImGui::SliderFloat("Terrain size", &terrainSize, 10.f, 1000.f, "%.0f");
        if (ImGui::SliderInt("Tessellation", &tessellation, 2, 2048)) {
            terrain.generateMesh(tessellation);
        }
        if (ImGui::Button("Randomize seed")) {
            randomSeed = (float) (rand() % 1000);
        }
    }

    if (ImGui::CollapsingHeader("Camera", "camera_ch", true, true)) {
        ImGui::SliderFloat("Camera rotation speed", &rotation_speed, 0.f, 50.f, "%.0f");
        ImGui::SliderFloat("Camera movement speed", &cameraSpeed, 10.f, 100.f, "%.0f");
    }

    if (ImGui::CollapsingHeader("Light", "light_ch", true, true)) {
        ImGui::SliderFloat("Environment multiplier", &environment_multiplier, 0.0f, 10.0f);
        ImGui::ColorEdit3("Point light color", &point_light_color.x);
        ImGui::SliderFloat("Point light intensity multiplier", &point_light_intensity_multiplier, 0.0f,
                           30000.0f, "%.3f", 2.f);
        ImGui::Checkbox("Manual light only (right-click drag to move)", &lightManualOnly);
    }

    if (ImGui::Button("Reload Shaders")) {
        loadShaders(true);
    }

    // ----------------------------------------------------------
    // Render the GUI.
    ImGui::Render();
}

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    g_window = labhelper::init_window_SDL("OpenGL Project");

    initGL();

    bool stopRendering = false;
    auto startTime = std::chrono::system_clock::now();

    while (!stopRendering) {
        //update currentTime
        std::chrono::duration<float> timeSinceStart = std::chrono::system_clock::now() - startTime;
        previousTime = currentTime;
        currentTime = timeSinceStart.count();
        deltaTime = currentTime - previousTime;
        // render to window
        display();

        // Render overlay GUI.
        if (showUI) {
            gui();
        }

        // Swap front and back buffer. This frame will now been displayed.
        SDL_GL_SwapWindow(g_window);

        // check events (keyboard among other)
        stopRendering = handleEvents();
    }
    // Free Models
    labhelper::freeModel(sphereModel);

    // Shut down everything. This includes the window and all other subsystems.
    labhelper::shutDown(g_window);
    return 0;
}
