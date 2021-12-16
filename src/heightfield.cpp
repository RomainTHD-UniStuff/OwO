
#include "heightfield.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>

using namespace glm;
using std::string;

/// generate a mesh in range -1 to 1 in x and z
/// (y is 0 but will be altered in height field vertex shader)
void HeightField::generateMesh(int p_tessellation) noexcept {
    this->tessellation = p_tessellation;

    glGenBuffers(1, &this->positionBuffer);
    glGenBuffers(1, &this->uvBuffer);
    glGenBuffers(1, &this->indexBuffer);

    glGenVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);

    positions.clear();
    texCoords.clear();
    indices.clear();

    positions.reserve((tessellation + 1) * (tessellation + 1) * 3);
    texCoords.reserve((tessellation + 1) * (tessellation + 1) * 2);
    indices.reserve(tessellation * tessellation * 2 * 3);

    for (int z = 0; z <= tessellation; ++z) {
        for (int x = 0; x <= tessellation; ++x) {
            positions.push_back(2.f * (float) x / ((float) tessellation) - 1.f); // x
            positions.push_back(0.f);                                            // y
            positions.push_back(2.f * (float) z / ((float) tessellation) - 1.f); // z

            texCoords.push_back((float) x / ((float) tessellation)); // u
            texCoords.push_back((float) z / ((float) tessellation)); // v
        }
    }

    for (int z = 0; z < tessellation; ++z) {
        for (int x = 0; x <= tessellation; ++x) {
            indices.push_back(x + z * (tessellation + 1));
            indices.push_back(x + (z + 1) * (tessellation + 1));
        }

        indices.push_back(UINT32_MAX);
    }

    // Positions
    glBindBuffer(GL_ARRAY_BUFFER, this->positionBuffer);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), &positions[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, this->uvBuffer);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), &texCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, nullptr);
    glEnableVertexAttribArray(2);

    // Triangle indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
}

void HeightField::submitTriangles(bool linesOnly) const noexcept {
    if (vao == UINT32_MAX) {
        std::cout << "No vertex array is generated, cannot draw anything.\n";
        return;
    }

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(UINT32_MAX);

    glBindVertexArray(this->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBuffer);

    if (linesOnly) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glDrawElements(GL_TRIANGLE_STRIP, this->indices.size(), GL_UNSIGNED_INT, nullptr);

    if (linesOnly) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}
