
#include <GL/glew.h>

// STB_IMAGE for loading images of many filetypes
#include <stb_image.h>

#include <cstdlib>

#include <labhelper.h>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

// The window we'll be rendering to
SDL_Window* g_window = nullptr;

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;

int mag = GL_LINEAR;
int mini = GL_LINEAR_MIPMAP_LINEAR;
float anisotropy = 16.0f;
float camera_pan = 0.f;
bool showUI = true;

// The shaderProgram holds the vertexShader and fragmentShader
GLuint shaderProgram;

// The vertexArrayObject here will hold the pointers to
// the vertex data (in roadPositionBuffer) and color data per vertex (in colorBuffer)
GLuint vertexArrayObject;

GLuint roadPositionBuffer,
    roadIndexBuffer,
    roadTexBuffer,
    roadTexture;

GLuint explosionPositionBuffer,
    explosionIndexBuffer,
    explosionTexBuffer,
    explosionTexture;

void initRoad() {
    ///////////////////////////////////////////////////////////////////////////
    // Create the positions buffer object
    ///////////////////////////////////////////////////////////////////////////
    const float positions[] = {
        // X      Y       Z
        -10.0f, -5.0f, -10.0f,  // v0
        -10.0f, 100.0f, -330.0f, // v1
        10.0f, 100.0f, -330.0f, // v2
        10.0f, -5.0f, -10.0f   // v3
    };
    // Create a handle for the vertex position buffer
    glGenBuffers(1, &roadPositionBuffer);
    // Set the newly created buffer as the current one
    glBindBuffer(GL_ARRAY_BUFFER, roadPositionBuffer);
    // Send the vertex position data to the current buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, nullptr /*offset*/);
    // Enable the attribute
    glEnableVertexAttribArray(0);

    ///////////////////////////////////////////////////////////////////////////
    // >>> @task 1 : Create the texture coordinates.
    //				 Create the texture coordinates' buffer object.
    //				 Set up the attrib pointer.
    //				 Enable the vertex attrib array.
    ///////////////////////////////////////////////////////////////////////////

    float texCoords[] = {
        0.0f, 0.0f, // (u,v) for v0
        0.0f, 15.0f, // (u,v) for v1
        1.0f, 15.0f, // (u,v) for v2
        1.0f, 0.0f // (u,v) for v3
    };

    glGenBuffers(1, &roadTexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, roadTexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, nullptr/*offset*/);
    glEnableVertexAttribArray(2);

    ///////////////////////////////////////////////////////////////////////////
    // Create the element array buffer object
    ///////////////////////////////////////////////////////////////////////////
    const int indices[] = {
        0, 1, 3, // Triangle 1
        1, 2, 3  // Triangle 2
    };
    glGenBuffers(1, &roadIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, roadIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glGenTextures(1, &roadTexture);
    glBindTexture(GL_TEXTURE_2D, roadTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //************************************
    //			Load Texture
    //************************************
    // >>> @task 2

    int w, h, comp;
    unsigned char* image = stbi_load("../scenes/asphalt.jpg", &w, &h, &comp, STBI_rgb_alpha);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(image);
}

void initExplosion() {
    const float positions[] = {
        // X      Y       Z
        -1.0f, -1.0f, -10.0f,  // v0
        -1.0f, 1.0f, -10.0f, // v1
        1.0f, 1.0f, -10.0f, // v2
        1.0f, -1.0f, -10.0f   // v3
    };

    // Create a handle for the vertex position buffer
    glGenBuffers(1, &explosionPositionBuffer);
    // Set the newly created buffer as the current one
    glBindBuffer(GL_ARRAY_BUFFER, explosionPositionBuffer);
    // Send the vertex position data to the current buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, false /*normalized*/, 0 /*stride*/, nullptr /*offset*/);
    // Enable the attribute

    glEnableVertexAttribArray(0);

    float texCoords[] = {
        0.0f, 0.0f, // (u,v) for v0
        0.0f, 1.0f, // (u,v) for v1
        1.0f, 1.0f, // (u,v) for v2
        1.0f, 0.0f // (u,v) for v3
    };

    glGenBuffers(1, &explosionTexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, explosionTexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

    glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, nullptr/*offset*/);

    glEnableVertexAttribArray(2);

    const int indices[] = {
        0, 1, 3, // Triangle 1
        1, 2, 3  // Triangle 2
    };
    glGenBuffers(1, &explosionIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, explosionIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glGenTextures(1, &explosionTexture);
    glBindTexture(GL_TEXTURE_2D, explosionTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    int w, h, comp;
    unsigned char* image = stbi_load("../scenes/explosion.png", &w, &h, &comp, STBI_rgb_alpha);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    free(image);
}

void initGL() {
    ///////////////////////////////////////////////////////////////////////////
    // Create the vertex array object
    ///////////////////////////////////////////////////////////////////////////
    // Create a handle for the vertex array object
    glGenVertexArrays(2, &vertexArrayObject); // FIXME: ?
    // Set it as current, i.e., related calls will affect this object
    glBindVertexArray(vertexArrayObject);

    initRoad();
    initExplosion();

    // The loadShaderProgram and linkShaderProgram functions are defined in glutil.cpp and
    // do exactly what we did in lab1 but are hidden for convenience
    shaderProgram = labhelper::loadShaderProgram("../lab2-textures/simple.vert",
                                                 "../lab2-textures/simple.frag");
}

void display() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mini);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

    // The viewport determines how many pixels we are rasterizing to
    int w, h;
    SDL_GetWindowSize(g_window, &w, &h);
    // Set viewport
    glViewport(0, 0, w, h);

    // Set clear color
    glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
    // Clears the color buffer and the z-buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // We disable backface culling for this tutorial, otherwise care must be taken with the winding order
    // of the vertices. It is however a lot faster to enable culling when drawing large scenes.
    glDisable(GL_CULL_FACE);
    // Disable depth testing
    glDisable(GL_DEPTH_TEST);
    // Set the shader program to use for this draw call
    glUseProgram(shaderProgram);

    // Set up a projection matrix
    float fovy = radians(45.0f);
    float aspectRatio = float(w) / float(h);
    float nearPlane = 0.01f;
    float farPlane = 300.0f;
    mat4 projectionMatrix = perspective(fovy, aspectRatio, nearPlane, farPlane);
    // Send it to the vertex shader
    int loc = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(loc, 1, false, &projectionMatrix[0].x);

    loc = glGetUniformLocation(shaderProgram, "cameraPosition");
    glUniform3f(loc, camera_pan, 0, 0);

    // >>> @task 3.1

    glBindVertexArray(vertexArrayObject);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, roadTexture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, explosionTexture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glUseProgram(0); // "unsets" the current shader program. Not really necessary.
}

void gui() {
    // Inform imgui of new frame
    ImGui_ImplSdlGL3_NewFrame(g_window);

    // ----------------- Set variables --------------------------
    ImGui::PushID("mag");
    ImGui::Text("Magnification");
    ImGui::RadioButton("GL_NEAREST", &mag, GL_NEAREST);
    ImGui::RadioButton("GL_LINEAR", &mag, GL_LINEAR);
    ImGui::PopID();

    ImGui::PushID("mini");
    ImGui::Text("Minification");
    ImGui::RadioButton("GL_NEAREST", &mini, GL_NEAREST);
    ImGui::RadioButton("GL_LINEAR", &mini, GL_LINEAR);
    ImGui::RadioButton("GL_NEAREST_MIPMAP_NEAREST", &mini, GL_NEAREST_MIPMAP_NEAREST);
    ImGui::RadioButton("GL_NEAREST_MIPMAP_LINEAR", &mini, GL_NEAREST_MIPMAP_LINEAR);
    ImGui::RadioButton("GL_LINEAR_MIPMAP_NEAREST", &mini, GL_LINEAR_MIPMAP_NEAREST);
    ImGui::RadioButton("GL_LINEAR_MIPMAP_LINEAR", &mini, GL_LINEAR_MIPMAP_LINEAR);
    ImGui::PopID();

    ImGui::SliderFloat("Anisotropic filtering", &anisotropy, 1.0, 16.0, "Number of samples: %.0f");
    ImGui::Dummy({0, 20});
    ImGui::SliderFloat("Camera Panning", &camera_pan, -1.0, 1.0);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    // ----------------------------------------------------------


    // Render the GUI.
    ImGui::Render();
}

int main(int argc, char* argv[]) {
    g_window = labhelper::init_window_SDL("OpenGL Lab 2");

    initGL();

    // render-loop
    bool stopRendering = false;
    while (!stopRendering) {
        // render to window
        display();

        // Render overlay GUI.
        if (showUI) {
            gui();
        }

        // Swap front and back buffer. This frame will now been displayed.
        SDL_GL_SwapWindow(g_window);

        // check events (keyboard among other)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Allow ImGui to capture events.
            ImGui_ImplSdlGL3_ProcessEvent(&event);

            if (event.type == SDL_QUIT || (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_ESCAPE)) {
                stopRendering = true;
            }
            if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g) {
                showUI = !showUI;
            }
        }
    }

    // Shut down everything. This includes the window and all other subsystems.
    labhelper::shutDown(g_window);
    return 0;
}
