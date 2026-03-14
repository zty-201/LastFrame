#version 320 es
precision highp float;
precision highp usampler2D;
precision highp sampler2D;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec4 VS_IN_Position;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform mat4 u_LightMatrix;
uniform int u_VisibleIndexOffset;
uniform highp sampler2D s_InstanceData;
uniform highp usampler2D s_VisibleIndices;

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

void main()
{
    uint actualInstanceID = fetchVisibleIndex(u_VisibleIndexOffset + gl_InstanceID);
    int baseTexel = int(actualInstanceID) * 6;
    mat4 model = mat4(
        fetchInstanceData(baseTexel + 1),
        fetchInstanceData(baseTexel + 2),
        fetchInstanceData(baseTexel + 3),
        fetchInstanceData(baseTexel + 4)
    );  

    // 3. Transform
    gl_Position = u_LightMatrix * model * vec4(VS_IN_Position.xyz, 1.0f);
}