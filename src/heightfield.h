#include <string>
#include <GL/glew.h>
#include <vector>

/**
 * Heightfield
 */
class HeightField {
public:
    /**
     * Default constructor
     */
    HeightField() = default;

    /**
     * Generate the mesh
     * @param tessellation Tessellation level, the number of "squares" per side
     */
    void generateMesh(int tessellation) noexcept;

    /**
     * Display the mesh
     * @param linesOnly Render only the lines
     */
    void submitTriangles(bool linesOnly) const noexcept;

private:
    //-------------------------------------------------------------------------
    // OpenGL variables
    //-------------------------------------------------------------------------

    /**
     * VAO
     */
    GLuint vao {UINT32_MAX};

    /**
     * Position buffer
     */
    GLuint positionBuffer {UINT32_MAX};

    /**
     * UV buffer
     */
    GLuint uvBuffer {UINT32_MAX};

    /**
     * Index buffer
     */
    GLuint indexBuffer {UINT32_MAX};

    //-------------------------------------------------------------------------
    // Inner variables
    //-------------------------------------------------------------------------

    /**
     * List of triangles positions (x, 0, z)
     */
    std::vector<float> positions;

    /**
     * List of corresponding texture coordinates between [0, 1[
     */
    std::vector<float> texCoords;

    /**
     * Triangles indices
     */
    std::vector<uint32_t> indices;

    /**
     * Tesselation level
     */
    int tessellation {0};
};
