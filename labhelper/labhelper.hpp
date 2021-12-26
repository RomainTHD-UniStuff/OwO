#ifndef LAB_GL_UTILS_H
#define LAB_GL_UTILS_H

#include <glm/glm.hpp>

#include <string>
#include <cassert>

#include <SDL.h>

#undef main

#include <GL/glew.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * Check for GL errors using glGetError().
 *
 * If a debugger is attached, CHECK_GL_ERROR() will break on the offending line, and print the file and line location
 * in an MSVC compatible format on the debug output and console.
 *
 * CHECK_GL_ERROR() will report any errors since the last call to glGetError() !
 * If CHECK_GL_ERROR() reports an error, you *must* consider all calls to GL since the last CHECK_GL_ERROR()
 * (or call to glGetError()) !
 *
 * Note: the macro _cannot_ be used between glBegin() and glEnd(), as stated in
 * the OpenGL standard.
 * 
 * Example (we're looking for an error at either glClearColor() or glClear()): 
 *    CHECK_GL_ERROR(); // catch previous errors
 *    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
 *    CHECK_GL_ERROR(); // see if glClearColor() generated an error
 *    glClear(GL_COLOR_BUFFER_BIT);
 *    CHECK_GL_ERROR(); // see if glClear() generated an error
 */
#define CHECK_GL_ERROR() { \
    owo::checkGLError(__FILE__, __LINE__) && (__debugbreak(), 1); \
}

namespace owo {

#if !defined(_WIN32)
#define __debugbreak() assert(false)
#endif

    /**
     * Internal function used by macro CHECK_GL_ERROR, use that instead.
     */
    bool checkGLError(const char* file, int line);

    /**
     * Print GL version/vendor and renderer. Ensure that we've got OpenGL 3.0 (GLEW_VERSION_3_0). Bail if we don't.
     */
    void startupGLDiagnostics();

    /**
     * Initialize OpenGL debug messages.
     */
    void setupGLDebugMessages();

    /**
     * Error reporting function
     */
    void fatal_error(std::string errorString, std::string title = std::string());

    void non_fatal_error(std::string errorString, std::string title = std::string());

    /**
     * Initialize a window, an openGL context, and initiate async debug output.
     */
    SDL_Window* init_window_SDL(std::string caption, int width = 1280, int height = 720);

    /**
     * Destroys that which have been initialized.
     */
    void shutDown(SDL_Window* window);

    /**
     * Helper function: creates a cube map using the files specified for each face.
     */
    GLuint loadCubeMap(const char* facePosX, const char* faceNegX, const char* facePosY, const char* faceNegY,
                       const char* facePosZ, const char* faceNegZ);

    /**
     * Helper function used to get log info (such as errors) about a shader object or shader program
     */
    std::string GetShaderInfoLog(GLuint obj);

    /**
     * Loads and compiles a fragment and vertex shader. Then creates a shader program
     * and attaches the shaders. Does NOT link the program, this is done with  linkShaderProgram()
     * The reason for this is that before linking we need to bind attribute locations, using
     * glBindAttribLocation and fragment data lications, using glBindFragDataLocation.
     */
    GLuint loadShaderProgram(const std::string& vertexShader,
                             const std::string& fragmentShader,
                             bool allow_errors = false);

    /**
     * Call to link a shader program prevoiusly loaded using loadShaderProgram.
     */
    bool linkShaderProgram(GLuint shaderProgram, bool allow_errors = false);

    /**
     * Creates a GL buffer and uploads the given data to it.
     * returns the handle of the GL buffer.
     */
    GLuint createAddAttribBuffer(GLuint vertexArrayObject,
                                 const void* data,
                                 size_t dataSize,
                                 GLuint attributeIndex,
                                 GLsizei attributeSize,
                                 GLenum type,
                                 GLenum bufferUsage = GL_STATIC_DRAW);

    /**
     * Helper to set uniform variables in shaders, labeled SLOW because they find the location from string each time.
     * In OpenGL (and similarly in other APIs) it is much more efficient (in terms of CPU time) to keep the uniform
     * location, and use that. Or even better, use uniform buffers!
     * However, in the simple tutorial samples, performance is not an issue.
     * Overloaded to set many types.
     */
    void setUniformSlow(GLuint shaderProgram, const char* name, const glm::vec2& value);

    void setUniformSlow(GLuint shaderProgram, const char* name, const glm::mat4& matrix);

    void setUniformSlow(GLuint shaderProgram, const char* name, float value);

    void setUniformSlow(GLuint shaderProgram, const char* name, GLint value);

    void setUniformSlow(GLuint shaderProgram, const char* name, const glm::vec3& value);

    void setUniformSlow(GLuint shaderProgram, const char* name, uint32_t nof_values, const glm::vec3* values);

    /**
     * Helper to draw a single quad (two triangles) that cover the entire screen
     */
    void drawFullScreenQuad();

    /**
     * Code that draws a sphere where the light is and a stippled line to the
     * origo. Note that this method uses functions that have been deprecated in
     * OpenGL 3 and so would not necessarily work on all future implementations.
     *
     * That said, for drawing simple debug information such as this, OpenGL 2
     * can be pretty handy.
     */
    void debugDrawLine(const glm::mat4& viewMatrix,
                       const glm::mat4& projectionMatrix,
                       const glm::vec3& worldSpaceLightPos);

    /**
     * @return Random, uniformly distributed floating point numbers in the interval [from, to].
     */
    float uniform_randf(float from, float to);

    /**
     * @return Random, uniformly distributed floating point numbers in the interval [0, 1].
     */
    float randf();

    /**
     * Generate uniform points on a disc
     */
    void concentricSampleDisk(float* dx, float* dy);

    /**
     * Generate points with a cosine distribution on the hemisphere
     */
    glm::vec3 cosineSampleHemisphere();
}

#endif // LAB_GL_UTILS_H
