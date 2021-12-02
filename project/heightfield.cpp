
#include "heightfield.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>

using namespace glm;
using std::string;

HeightField::HeightField() :
    m_meshResolution(0),
    m_vao(UINT32_MAX),
    m_positionBuffer(UINT32_MAX),
    m_uvBuffer(UINT32_MAX),
    m_indexBuffer(UINT32_MAX),
    m_numIndices(0),
    m_texid_hf(UINT32_MAX),
    m_texid_diffuse(UINT32_MAX) {
}

void HeightField::loadHeightField(const std::string& heightFieldPath) {
    int width, height, components;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(heightFieldPath.c_str(), &width, &height, &components, 1);
    if (data == nullptr) {
        std::cout << "Failed to load image: " << heightFieldPath << ".\n";
        return;
    }

    if (m_texid_hf == UINT32_MAX) {
        glGenTextures(1, &m_texid_hf);
    }
    glBindTexture(GL_TEXTURE_2D, m_texid_hf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT,
                 data); // just one component (float)

    m_heightFieldPath = heightFieldPath;
    std::cout << "Successfully loaded height field texture: " << heightFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string& diffusePath) {
    int width, height, components;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
    if (data == nullptr) {
        std::cout << "Failed to load image: " << diffusePath << ".\n";
        return;
    }

    if (m_texid_diffuse == UINT32_MAX) {
        glGenTextures(1, &m_texid_diffuse);
    }

    glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
    glGenerateMipmap(GL_TEXTURE_2D);

    std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}

void HeightField::generateMesh(int tesselation) {
    // generate a mesh in range -1 to 1 in x and z
    // (y is 0 but will be altered in height field vertex shader)

    glGenVertexArrays(1, &this->m_vao);
    glBindVertexArray(this->m_vao);

    positions.reserve((tesselation + 1) * (tesselation + 1) * 3);
    texCoords.reserve((tesselation + 1) * (tesselation + 1) * 2);
    indices.reserve(tesselation * tesselation * 2 * 3);

    for (int z = 0; z <= tesselation; ++z) {
        for (int x = 0; x <= tesselation; ++x) {
            positions.push_back(2.f * (float) x / ((float) tesselation) - 1.f); // x
            positions.push_back(0.f);                                           // y
            positions.push_back(2.f * (float) z / ((float) tesselation) - 1.f); // z

            texCoords.push_back((float) x / ((float) tesselation)); // u
            texCoords.push_back((float) z / ((float) tesselation)); // v
        }
    }

    for (int z = 0; z < tesselation; ++z) {
        for (int x = 0; x < tesselation; ++x) {
            // Top triangle
            indices.push_back(x + z * (tesselation + 1));
            indices.push_back(x + 1 + z * (tesselation + 1));
            indices.push_back(x + 1 + (z + 1) * (tesselation + 1));

            // Bottom triangle
            indices.push_back(x + z * (tesselation + 1));
            indices.push_back(x + (z + 1) * (tesselation + 1));
            indices.push_back(x + 1 + (z + 1) * (tesselation + 1));
        }
    }

    // Positions
    glGenBuffers(1, &this->m_positionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &positions[0], GL_STATIC_DRAW);

    // Texture coordinates
    glGenBuffers(1, &this->m_uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), &texCoords[0], GL_STATIC_DRAW);

    // Triangle indices
    glGenBuffers(1, &this->m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
}

void HeightField::submitTriangles() const {
    if (m_vao == UINT32_MAX) {
        std::cout << "No vertex array is generated, cannot draw anything.\n";
        return;
    }

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(UINT32_MAX);

    glBindVertexArray(this->m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, this->m_positionBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_indexBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, this->m_uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), &texCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(2);

    glDrawElements(GL_TRIANGLE_STRIP, this->indices.size(), GL_UNSIGNED_SHORT, nullptr);
}
