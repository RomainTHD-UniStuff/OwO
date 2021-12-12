#version 420

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
float material_reflectivity = 0.;
float material_metalness = 0.;
float material_fresnel = 0.;
float material_shininess = 0.;
float material_emission = 0.5;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color;
uniform float point_light_intensity_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec3 viewSpaceNormal;
in vec3 viewSpacePosition;
in float yPos;
in float colorBleeding;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;

uniform int has_color_texture;
layout(binding = 0) uniform sampler2D colorMap;

vec3 calculateDirectIllumiunation(vec3 wo, vec3 n, vec3 base_color) {
    vec3 direct_illum = base_color;

    float d = distance(viewSpaceLightPosition, viewSpacePosition);
    vec3 Li = point_light_intensity_multiplier * point_light_color * 1/(d*d);

    vec3 wi = normalize(viewSpaceLightPosition - viewSpacePosition);

    if (dot(n, wi) <= 0.) {
        return vec3(0., 0., 0.);
    } else if (isnan(dot(n, wi))) {
        return Li / 10.;
    }

    vec3 diffuse_term = direct_illum * 1.0 / PI * abs(dot(n, wi)) * Li;

    vec3 wh = normalize(wi + wo);

    float R = material_fresnel;
    float F = R + (1. - R) * pow(1. - dot(wh, wi), 5.);

    float s = material_shininess;
    float D = (s + 2.) / (2. * PI) * pow(max(0.0001, dot(n, wh)), s);

    float G = min(1., min(2. * dot(n, wh) * dot(n, wo) / dot(wo, wh), 2. * dot(n, wh) * dot(n, wi) / dot(wo, wh)));

    float brdf = F * D * G / (4. * dot(n, wo) * dot(n, wi));

    vec3 dielectric_term = brdf * dot(n, wi) * Li + (1 - F) * diffuse_term;

    float m = material_metalness;
    vec3 metal_term = brdf * base_color * dot(n, wi) * Li;
    vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;

    float r = material_reflectivity;

    return r * microfacet_term + (1 - r) * diffuse_term;
}

vec3 calculateIndirectIllumination(vec3 wo, vec3 n, vec3 base_color) {
    vec3 indirect_illum = vec3(0.f);

    vec4 dir = viewInverse * vec4(n.x, n.y, n.z, 0.);

    // Calculate the spherical coordinates of the direction
    float theta = acos(max(-1.0f, min(1.0f, dir.y)));
    float phi = atan(dir.z, dir.x);
    if (phi < 0.0f) {
        phi = phi + 2.0f * PI;
    }

    vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);

    vec4 irradiance = texture(irradianceMap, lookup);

    vec3 diffuse_term = base_color * (1.0 / PI) * vec3(irradiance);

    indirect_illum = diffuse_term;

    float s = material_shininess;
    float roughness = sqrt(sqrt(2. / (s + 2.)));
    vec3 Li = environment_multiplier * textureLod(reflectionMap, lookup, roughness * 7.0).xyz;

    vec3 wi = reflect(normalize(viewSpaceLightPosition - viewSpacePosition), vec3(dir));
    vec3 wh = normalize(wi + wo);
    float R = material_fresnel;
    float F = R + (1. - R) * pow(1. - dot(wh, wi), 5.);

    vec3 dielectric_term = F * Li + (1. - F) * diffuse_term;
    vec3 metal_term = F * base_color * Li;

    float m = material_metalness;
    vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;

    float r = material_reflectivity;
    indirect_illum = r * microfacet_term + (1 - r) * diffuse_term;

    return indirect_illum;
}

const vec4[] colors = {
    // Snow high
    vec4(255., 255., 255., 1.),
    // Snow low
    vec4(240., 240., 240., 1.),

    // Rock high
    vec4(132., 132., 132., 1.),
    // Rock low
    vec4(122., 122., 122., 1.),

    // Mountain high
    vec4(107., 85., 68., 1.),
    // Mountain med
    vec4(97., 75., 58., 1.),
    // Mountain low
    vec4(90., 68., 50., 1.),

    // Forest high
    vec4(32., 72., 29., 1.),
    // Forest low
    vec4(42., 82., 39., 1.),

    // Plain high
    vec4(47., 87., 45., 1.),
    // Plain mid-high
    vec4(52., 92., 50., 1.),
    // Plain mid-low
    vec4(58., 100., 54., 1.),
    // Plain low
    vec4(70., 115., 63., 1.),

    // Beach
    vec4(224., 205., 169., 1.),

    // Ocean high
    vec4(33., 90., 113., 0.5),
    // Ocean med
    vec4(23., 85., 112., 0.5),
    // Ocean low
    vec4(16., 80., 110., 0.5),

    // Deep ocean high
    vec4(12., 77., 114., 0.5),
    // Deep ocean med
    vec4(9., 59., 87., 0.5),
    // Deep ocean low
    vec4(2., 45., 71., 0.5),
};

vec4 colorFromAltitude() {
    float y = yPos;
    float cb = colorBleeding;
    if (y >= 0) {
        y = max(0, y + y * cb * 10);
    } else {
        y = min(-0.001, y - y * cb * 10);
    }

    y = exp(y);
    int n = 0;

    if (y < 0.4) {
        // Deep ocean low
        ++n;
    }
    if (y < 0.6) {
        // Deep ocean med
        ++n;
    }
    if (y < 0.7) {
        // Deep ocean high
        ++n;
    }

    if (y < 0.8) {
        // Ocean low
        ++n;
    }
    if (y < 0.9) {
        // Ocean med
        ++n;
    }
    if (y < 1.) {
        // Ocean high
        ++n;
    }

    if (y < 1.1) {
        // Beach
        ++n;
    }

    if (y < 1.3) {
        // Plain low
        ++n;
    }
    if (y < 1.4) {
        // Plain mid-low
        ++n;
    }
    if (y < 1.5) {
        // Plain mid-high
        ++n;
    }
    if (y < 1.7) {
        // Plain high
        ++n;
    }

    if (y < 1.9) {
        // Forest low
        ++n;
    }
    if (y < 2.1) {
        // Forest high
        ++n;
    }

    if (y < 2.4) {
        // Mountain low
        ++n;
    }
    if (y < 2.6) {
        // Mountain med
        ++n;
    }
    if (y < 3.) {
        // Mountain high
        ++n;
    }

    if (y < 3.3) {
        // Rock low
        ++n;
    }
    if (y < 3.7) {
        // Rock high
        ++n;
    }

    if (y < 4) {
        // Snow low
        ++n;
    }

    // Snow high
    ++n;

    return colors[n - 1];
}

void main() {
    vec3 dx = dFdx(viewSpacePosition);
    vec3 dy = dFdy(viewSpacePosition);
    vec3 n = normalize(cross(dx, dy));

    vec3 wo = normalize(-viewSpacePosition);

    vec3 base_color = colorFromAltitude().rgb;
    base_color /= 255.;

    vec3 direct_illumination_term = calculateDirectIllumiunation(wo, n, base_color);

    vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n, base_color);

    vec3 emission_term = material_emission * base_color;

    vec3 final_color = direct_illumination_term + indirect_illumination_term + emission_term;

    // Check if we got invalid results in the operations
    if (any(isnan(final_color))) {
        final_color.xyz = vec3(1.f, 0.f, 0.f);
    }

    fragmentColor.xyz = final_color;
}
