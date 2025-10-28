#version 310 es
precision highp float;

layout(location=0) in vec3  aPos;
layout(location=1) in vec3  aNormal;
layout(location=2) in vec3  aTangent;
layout(location=3) in vec2  aUV;
layout(location=4) in vec4  aWeights;    // normalized from u8 -> [0,1]
layout(location=5) in uvec4 aBones;      // indices (u8 -> uint)

layout(std140) uniform Skinning {
    mat4 uBones[256];
};

uniform mat4 uViewProj;

void main() {
    // linear blend skinning (LBS)
    mat4 skin =
          aWeights.x * uBones[aBones.x] +
          aWeights.y * uBones[aBones.y] +
          aWeights.z * uBones[aBones.z] +
          aWeights.w * uBones[aBones.w];

    vec4 wp = skin * vec4(aPos, 1.0);
    gl_Position = uViewProj * wp;

    // If you need normals/tangents:
    mat3 rot = mat3(skin);
    vec3 N = normalize(rot * aNormal);
    vec3 T = normalize(rot * aTangent);
    // pass to fragment shader as needed
}