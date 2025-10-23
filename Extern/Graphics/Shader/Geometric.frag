#version 310 es

precision highp float;
precision highp int;

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
in mat3 TBN;

// G-buffer outputs
layout(location=0) out vec3 gPosition;
layout(location=1) out vec3 gNormal;
layout(location=2) out vec4 gAlbedoSpec; // RGB = albedo, A = specular intensity

struct Material {
    float rough;
    float metal;
    vec3 color;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
    float effect;          
};

uniform Material material;
uniform bool UseTexture;
uniform bool UseNormalMap;

layout(binding=0) uniform sampler2D DiffuseTex;
layout(binding=2) uniform sampler2D NormalMap;

void main() {
    // 1. Store position in view space
    gPosition = Position;
    
    // 2. Calculate normal (including normal mapping if enabled)
    vec3 normal;
    if (UseNormalMap) {
        vec3 normalMap = texture(NormalMap, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0; // Convert [0,1] to [-1,1]
        normal = normalize(TBN * normalMap);
    } else {
        normal = normalize(Normal);
    }
    gNormal = normal;
    
    // 3. Store albedo & material properties
    vec3 albedo = material.color;
    if (UseTexture) {
        vec4 texColor = texture(DiffuseTex, TexCoord);
        albedo = texColor.rgb;
    }
    
    float specular = material.metal; // Store metallicity in alpha channel
    gAlbedoSpec = vec4(albedo, specular);
    
    // Note: We can't handle effects like discard and checkerboard here,
    // those will need to be handled in the forward pass
}