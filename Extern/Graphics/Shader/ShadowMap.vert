#version 320 es
precision highp float;
// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec4 VS_IN_Position;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(std140, binding = 0) uniform Uniforms
{
    mat4  view;
    mat4  projection;
    mat4  view_proj;
    mat4  prev_view_proj;
    mat4  light_view_proj;
    mat4  inv_view_proj;
    vec4  light_direction;
    vec4  light_color;
    vec4  camera_position;
    vec4  bias_near_far_pow;
    vec4  aniso_density_scattering_absorption;
    vec4  time;
    ivec4 width_height;
};

// ------------------------------------------------------------------
// SSBO -------------------------------------------------------------
// ------------------------------------------------------------------
struct InstanceData {
    mat4 modelMatrix;
    vec4 tintColor;
    vec3 Ka;
    float roughness;
    vec3 Kd;
    float metallic;
    vec3 Ks;
    float shininess;
    uint effect;
    uint padding[3];
};

layout(std430, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    // --- GET INSTANCE DATA ---
    InstanceData instance = instances[gl_InstanceID];
    mat4 model = instance.modelMatrix;
    // -------------------------

    gl_Position = light_view_proj * model * vec4(VS_IN_Position.xyz, 1.0f);
}