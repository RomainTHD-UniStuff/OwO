#include <string>
#include <GL/glew.h>
#include <vector>

class HeightField {
public:
    int m_meshResolution; // triangles edges per quad side
    GLuint m_texid_hf;
    GLuint m_texid_diffuse;
    GLuint m_vao;
    GLuint m_positionBuffer;
    GLuint m_uvBuffer;
    GLuint m_indexBuffer;
    GLuint m_numIndices;
    GLuint m_normalsBuffer;
    std::string m_heightFieldPath;
    std::string m_diffuseTexturePath;

    HeightField();

    // load height field
    void loadHeightField(const std::string& heightFieldPath);

    // load diffuse map
    void loadDiffuseTexture(const std::string& diffusePath);

    // generate mesh
    void generateMesh(int tessellation);

    // render height map
    void submitTriangles(bool linesOnly) const;

private:
    std::vector<float> positions;
    std::vector<float> texCoords;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    int tessellation{};
};