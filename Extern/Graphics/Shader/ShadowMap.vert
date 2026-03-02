#version 320 es
precision highp float;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec4 VS_IN_Position;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform mat4 u_LightMatrix;

struct InstanceData {
    uint batchID;
    uint instanceOffset;
    uint padding[2];
    mat4 modelMatrix;
    vec4 boundingSphere;
};

layout(std430, binding = 2) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(std430, binding = 3) readonly buffer VisibleIndicesBuffer {
    uint visibleIndices[];
};

uniform int u_VisibleIndexOffset;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
// 1. Translate ID
    uint actualInstanceID = visibleIndices[u_VisibleIndexOffset + gl_InstanceID];

    // 2. Get Matrix
    mat4 model = instances[actualInstanceID].modelMatrix;

    // 3. Transform
    gl_Position = u_LightMatrix * model * vec4(VS_IN_Position.xyz, 1.0f);
}