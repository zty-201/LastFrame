#version 310 es

precision highp float;
precision highp int;

layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexNormal;
layout(location=2) in vec2 VertexTexCoord;
layout(location=3) in vec4 VertexTangent;

// For animation
layout(location=4) in vec4 BoneWeights; // from GL_UNSIGNED_BYTE, normalized
layout(location=5) in uvec4 BoneIndices; // from GL_UNSIGNED_BYTE

// Bone UBO
layout(std140, binding = 0) uniform Skinning {
    mat4 uBones[256];
};

// Toggle for skinning
uniform bool uUseSkinning;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Vec;
out mat3 TBN;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform int Pass;

void main()
{
    // Pass 0: Forward rendering for transparent objects and special effects
    if (Pass == 0) {
        TexCoord = VertexTexCoord;
        mat4 MV = V * M; 
        mat3 N = mat3(vec3(MV[0]), vec3(MV[1]), vec3(MV[2])); 
        
        Normal = normalize(N * VertexNormal);

        vec3 T = normalize(N * VertexTangent.xyz);
        vec3 B = cross(Normal, T) * VertexTangent.w;
        TBN = mat3(T, B, Normal);

        vec4 VertexPositionInView = MV * vec4(VertexPosition, 1.0f);
        Position = VertexPositionInView.xyz;
        gl_Position = P * VertexPositionInView; 
    } 
    // Pass 1: Skybox rendering
    else if (Pass == 1) {
        Vec = VertexPosition;
        mat4 viewNoTranslation = mat4(mat3(V));
        vec4 pos = P * viewNoTranslation * vec4(VertexPosition, 1.0);
        gl_Position = pos.xyzz;
    }
    // Pass 2: Debug wireframe rendering
    else {
        vec4 clip = P * V * M * vec4(VertexPosition, 1.0);
        clip.z -= 1e-3 * clip.w;
        gl_Position = clip;
    }
}