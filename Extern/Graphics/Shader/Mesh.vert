#version 320 es
precision highp float;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 VS_IN_Position;
layout(location = 1) in vec2 VS_IN_TexCoord;
layout(location = 2) in vec3 VS_IN_Normal;
layout(location = 3) in vec4 VS_IN_Tangent;
layout(location = 4) in ivec4 VS_IN_BoneIndices;
layout(location = 5) in vec4 VS_IN_BoneWeights;


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

layout(std140, binding = 0) uniform VolumetricUBO
{
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

uniform mat4 u_Model;
uniform vec4 u_UVTransform;

// For Animation
const int MAX_BONES = 128;

layout(std140, binding = 3) uniform SkinningMatrices
{
    mat4 uBone[MAX_BONES];
};

// ------------------------------------------------------------------
// HELPER FUNCTION --------------------------------------------------
// ------------------------------------------------------------------
mat4 ComputeSkinningMatrix()
{
    // If this vertex has no weights, return identity matrix
    float wSum = VS_IN_BoneWeights.x + VS_IN_BoneWeights.y + VS_IN_BoneWeights.z + VS_IN_BoneWeights.w;

    if (wSum < 0.0001)
    {
        return mat4(1.0);
    }

    mat4 skin = mat4(0.0);
    skin += uBone[VS_IN_BoneIndices.x] * VS_IN_BoneWeights.x;
    skin += uBone[VS_IN_BoneIndices.y] * VS_IN_BoneWeights.y;
    skin += uBone[VS_IN_BoneIndices.z] * VS_IN_BoneWeights.z;
    skin += uBone[VS_IN_BoneIndices.w] * VS_IN_BoneWeights.w;

    return skin;
}


// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{   
    // Skinning matrix
    mat4 skinning_mat = ComputeSkinningMatrix();

    mat4 model = u_Model * skinning_mat;

    vec4 world_pos = model * vec4(VS_IN_Position.xyz, 1.0f);
    FS_IN_WorldPos = world_pos.xyz;
    FS_IN_TexCoord = (VS_IN_TexCoord * u_UVTransform.xy) + u_UVTransform.zw;

    // Compute normal, tangent, bitangent in world space
    mat3 world3 = mat3(model);
    mat3 normal_mat = transpose(inverse(world3));

    vec3 skinnedNormal = normal_mat * VS_IN_Normal;
    vec3 skinnedTangent = normal_mat * VS_IN_Tangent.xyz;

    FS_IN_Normal    = normalize(skinnedNormal);
    FS_IN_Tangent   = normalize(skinnedTangent);
    FS_IN_Bitangent = cross(FS_IN_Normal, FS_IN_Tangent) * VS_IN_Tangent.w;

    gl_Position = view_proj * world_pos;
}