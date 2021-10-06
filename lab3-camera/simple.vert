#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
// incoming texcoord from the texcoord array
layout(location = 2) in vec2 texCoordIn;

// outgoing interpolated texcoord to fragshader
out vec2 texCoord;

uniform mat4 modelViewProjectionMatrix;

void main() {
    gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
    texCoord = texCoordIn;
}
