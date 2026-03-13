#version 320 es
precision highp float;
precision highp samplerBuffer;
precision highp usamplerBuffer;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
layout(location = 0) in vec3 a_Position;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------
uniform mat4 u_LightMatrix;
uniform int u_VisibleIndexOffset;
uniform samplerBuffer s_InstanceData;
uniform usamplerBuffer s_VisibleIndices; // 'u' because it's a uint texture
// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------
out vec3 FragPos;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------
void main() {
    // 1. Fetch the actual Instance ID using texelFetch
    uint actualInstanceID = texelFetch(s_VisibleIndices, u_VisibleIndexOffset + gl_InstanceID).r;

    // 2. Fetch the Model Matrix
    // Each InstanceData struct is 96 bytes (six vec4 texels).
    int baseTexel = int(actualInstanceID) * 6;
    
    // The matrix starts at texel offset 1 (skipping batchID/padding at offset 0)
    mat4 model = mat4(
        texelFetch(s_InstanceData, baseTexel + 1),
        texelFetch(s_InstanceData, baseTexel + 2),
        texelFetch(s_InstanceData, baseTexel + 3),
        texelFetch(s_InstanceData, baseTexel + 4)
    );

    // 3. Transform to World Space (For the Fragment Shader distance check)
    FragPos = vec3(model * vec4(a_Position, 1.0));

    // 4. Transform to Light Space
    gl_Position = u_LightMatrix * vec4(FragPos, 1.0);
}