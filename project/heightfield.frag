#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

uniform vec3 material_color;

in float yPos;
in float colorBleeding;
layout(location = 0) out vec4 fragmentColor;

// This simple fragment shader is meant to be used for debug purposes
// When the geometry is ok, we will migrate to use shading.frag instead.

vec4 colorFromAltitude() {
    float y = yPos;
    float cb = colorBleeding;
    if (y >= 0) {
        y = max(0, y + y * cb * 10);
    } else {
        y = min(-0.001, y - y * cb * 10);
    }

    if (y < -0.5) {
        return vec4(9., 59., 87., 0.5);
    } else if (y < 0.) {
        return vec4(23., 85., 112., 0.5);
    } else if (y < 0.05) {
        return vec4(224., 205., 169., 1.);
    } else if (y < 0.2) {
        return vec4(58., 100., 54., 1.);
    } else if (y < 0.5) {
        return vec4(132., 132., 132., 1.);
    } else {
        return vec4(240., 240., 240., 1.);
    }
}

void main() {
    vec4 color = colorFromAltitude();
    color = vec4(color.rgb / 255.f, 1.);
    fragmentColor = color;
}
