#version 320 es
precision highp float;
precision highp usampler2D;
precision highp sampler2D;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
layout(location = 0) in vec3 a_Position;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------
uniform mat4 u_LightMatrix;
uniform int u_VisibleIndexOffset;
uniform highp sampler2D s_InstanceData;
uniform highp usampler2D s_VisibleIndices;
// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------
out vec3 FragPos;

// ------------------------------------------------------------------
// HELPER -----------------------------------------------------------
// ------------------------------------------------------------------

vec4 fetchInstanceData(int index) {
    int x = index % 1024;
    int y = index / 1024;
    return texelFetch(s_InstanceData, ivec2(x, y), 0);
}

uint fetchVisibleIndex(int index) {
    int x = index % 2048;
    int y = index / 2048;
    return texelFetch(s_VisibleIndices, ivec2(x, y), 0).r;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------
void main() {
    uint actualInstanceID = fetchVisibleIndex(u_VisibleIndexOffset + gl_InstanceID);
    int baseTexel = int(actualInstanceID) * 6;
    mat4 model = mat4(
        fetchInstanceData(baseTexel + 1),
        fetchInstanceData(baseTexel + 2),
        fetchInstanceData(baseTexel + 3),
        fetchInstanceData(baseTexel + 4)
    );  

    // 3. Transform to World Space (For the Fragment Shader distance check)
    FragPos = vec3(model * vec4(a_Position, 1.0));

    // 4. Transform to Light Space
    gl_Position = u_LightMatrix * vec4(FragPos, 1.0);
}