#version 320 es
precision highp float;

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
layout(location = 0) in vec3 a_Position;

// ------------------------------------------------------------------
// DATA STRUCTURES & SSBOs
// ------------------------------------------------------------------
struct InstanceData {
    uint batchID;
    uint instanceOffset; 
    uint padding[2];
    mat4 modelMatrix;
    vec4 boundingSphere;
};

// Binding 2: Instance Data
layout(std430, binding = 2) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// Binding 3: Visible Indices
layout(std430, binding = 3) readonly buffer VisibleIndicesBuffer {
    uint visibleIndices[];
};

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------
uniform mat4 u_LightMatrix;
uniform int u_VisibleIndexOffset;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------
out vec3 FragPos;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------
void main() {
    // 1. Fetch the correct instance ID using the Fast Loop Uniform
    uint actualInstanceID = visibleIndices[u_VisibleIndexOffset + gl_InstanceID];

    // 2. Get the Model Matrix from the Mega-Buffer
    mat4 model = instances[actualInstanceID].modelMatrix;

    // 3. Transform to World Space (For the Fragment Shader distance check)
    FragPos = vec3(model * vec4(a_Position, 1.0));

    // 4. Transform to Light Space
    gl_Position = u_LightMatrix * vec4(FragPos, 1.0);
}