#version 320 es
precision highp float;
precision highp samplerBuffer;
precision highp usamplerBuffer;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------
layout(location = 0) in vec3 VS_IN_Position;
layout(location = 1) in vec2 VS_IN_TexCoord;
layout(location = 2) in vec3 VS_IN_Normal;
layout(location = 3) in vec4 VS_IN_Tangent;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_IN_WorldPos;
out vec3 FS_IN_Normal;
out vec2 FS_IN_TexCoord;
out vec3 FS_IN_Tangent;
out vec3 FS_IN_Bitangent;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(std140, binding = 0) uniform VolumetricUBO {
    mat4  view;
    mat4  projection;
    mat4  view_proj;
    mat4  prev_view_proj;
    mat4  inv_view_proj;
    mat4  cascade_view_proj[3];  // Array of cascade matrices
    vec4  cascade_splits;
    vec4  cascade_biases;
    vec4  light_direction;
    vec4  light_color;
    vec4  camera_position;
    vec4  bias_near_far_pow;
    vec4  aniso_density_scattering_absorption;
    vec4  width_height_time;
};

uniform int u_VisibleIndexOffset; 
uniform vec4 u_UVTransform;
uniform samplerBuffer s_InstanceData;
uniform usamplerBuffer s_VisibleIndices; // 'u' because it's a uint texture

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

    vec4 world_pos = model * vec4(VS_IN_Position, 1.0f);
    FS_IN_WorldPos = world_pos.xyz;
    FS_IN_TexCoord = (VS_IN_TexCoord * u_UVTransform.xy) + u_UVTransform.zw;

    mat3 normal_mat = transpose(inverse(mat3(model)));

    FS_IN_Normal    = normalize(normal_mat * VS_IN_Normal);
    FS_IN_Tangent   = normalize(normal_mat * VS_IN_Tangent.xyz);
    FS_IN_Bitangent = cross(FS_IN_Normal, FS_IN_Tangent) * VS_IN_Tangent.w;

    gl_Position = view_proj * world_pos;
}