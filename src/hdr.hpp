#include <vector>
#include <string>
#include <GL/glew.h>

namespace owo {
    GLuint loadHdrTexture(const std::string& filename);

    GLuint loadHdrMipmapTexture(const std::vector<std::string>& filenames);
}
