#version 320 es
precision highp float;
precision highp int; 
precision highp sampler2D; 
precision highp sampler3D;
precision highp sampler2DShadow;
#include <common.glsl>

// ------------------------------------------------------------------
// DEFINES  ---------------------------------------------------------
// ------------------------------------------------------------------

#define M_PI 3.14159265359
#define EPSILON 0.0001f

// ------------------------------------------------------------------
// OUTPUT VARIABLES  ------------------------------------------------
// ------------------------------------------------------------------

layout(location=0) out vec4 FS_OUT_Color;

// ------------------------------------------------------------------
// INPUT VARIABLES  -------------------------------------------------
// ------------------------------------------------------------------

in vec3 FS_IN_WorldPos;
in vec3 FS_IN_Normal;
in vec2 FS_IN_TexCoord;
in vec3 FS_IN_Tangent;
in vec3 FS_IN_Bitangent;

in flat float FS_IN_Roughness;
in flat float FS_IN_Metallic;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(std140, binding = 0) uniform VolumetricUBO
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

uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;
uniform sampler2D s_Roughness;
uniform sampler2D s_Metallic;   

uniform vec3  u_AlbedoFactor;
uniform float u_RoughnessFactor;
uniform float u_MetallicFactor;

layout(binding = 4) uniform sampler2DShadow s_ShadowMap;
layout(binding = 5) uniform sampler3D       s_VoxelGrid;

uniform bool u_Tricubic;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float sample_shadow_map(vec2 coord, float u, float v, float z, float inv_shadow_map_size)
{
    vec2 uv = coord + vec2(u, v) * inv_shadow_map_size;

    float current_depth = z;
    float bias          = bias_near_far_pow.x;

    return texture(s_ShadowMap, vec3(uv, current_depth - bias));
}

// ------------------------------------------------------------------

// http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
float visibility(vec3 p)
{
    // Transform frag position into Light-space.
    vec4 light_space_pos = light_view_proj * vec4(p, 1.0);

    // Perspective divide
    vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;

    // Transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;

    if (any(greaterThan(proj_coords.xy, vec2(1.0f))) || any(lessThan(proj_coords.xy, vec2(0.0f))))
        return 1.0f;

    float shadow_map_size     = 2048.0f;
    float inv_shadow_map_size = 1.0f / shadow_map_size;

    vec2 uv = proj_coords.xy * shadow_map_size; // 1 unit - 1 texel

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5, 0.5);
    base_uv *= inv_shadow_map_size;

    float uw0, uw1, uw2, uw3;
    uw0 = (5.0 * s - 6.0);
    uw1 = (11.0 * s - 28.0);
    uw2 = -(11.0 * s + 17.0);
    uw3 = -(5.0 * s + 1.0);

    float u0, u1, u2, u3;
    u0 = (4.0 * s - 5.0) / uw0 - 3.0;
    u1 = (4.0 * s - 16.0) / uw1 - 1.0;
    u2 = -(7.0 * s + 5.0) / uw2 + 1.0;
    u3 = -s / uw3 + 3.0;

    float vw0, vw1, vw2, vw3;
    vw0 = (5.0 * t - 6.0);
    vw1 = (11.0 * t - 28.0);
    vw2 = -(11.0 * t + 17.0);
    vw3 = -(5.0 * t + 1.0);

    float v0, v1, v2, v3;
    v0 = (4.0 * t - 5.0) / vw0 - 3.0;
    v1 = (4.0 * t - 16.0) / vw1 - 1.0;
    v2 = -(7.0 * t + 5.0) / vw2 + 1.0;
    v3 = -t / vw3 + 3.0;

    float sum = 0.0f;

    sum += uw0 * vw0 * sample_shadow_map(base_uv, u0, v0, proj_coords.z, inv_shadow_map_size);
    sum += uw1 * vw0 * sample_shadow_map(base_uv, u1, v0, proj_coords.z, inv_shadow_map_size);
    sum += uw2 * vw0 * sample_shadow_map(base_uv, u2, v0, proj_coords.z, inv_shadow_map_size);
    sum += uw3 * vw0 * sample_shadow_map(base_uv, u3, v0, proj_coords.z, inv_shadow_map_size);

    sum += uw0 * vw1 * sample_shadow_map(base_uv, u0, v1, proj_coords.z, inv_shadow_map_size);
    sum += uw1 * vw1 * sample_shadow_map(base_uv, u1, v1, proj_coords.z, inv_shadow_map_size);
    sum += uw2 * vw1 * sample_shadow_map(base_uv, u2, v1, proj_coords.z, inv_shadow_map_size);
    sum += uw3 * vw1 * sample_shadow_map(base_uv, u3, v1, proj_coords.z, inv_shadow_map_size);

    sum += uw0 * vw2 * sample_shadow_map(base_uv, u0, v2, proj_coords.z, inv_shadow_map_size);
    sum += uw1 * vw2 * sample_shadow_map(base_uv, u1, v2, proj_coords.z, inv_shadow_map_size);
    sum += uw2 * vw2 * sample_shadow_map(base_uv, u2, v2, proj_coords.z, inv_shadow_map_size);
    sum += uw3 * vw2 * sample_shadow_map(base_uv, u3, v2, proj_coords.z, inv_shadow_map_size);

    sum += uw0 * vw3 * sample_shadow_map(base_uv, u0, v3, proj_coords.z, inv_shadow_map_size);
    sum += uw1 * vw3 * sample_shadow_map(base_uv, u1, v3, proj_coords.z, inv_shadow_map_size);
    sum += uw2 * vw3 * sample_shadow_map(base_uv, u2, v3, proj_coords.z, inv_shadow_map_size);
    sum += uw3 * vw3 * sample_shadow_map(base_uv, u3, v3, proj_coords.z, inv_shadow_map_size);

    return sum * 1.0f / 2704.0f;
}

// ------------------------------------------------------------------

vec3 get_normal_from_map(vec3 tangent, vec3 bitangent, vec3 normal, vec2 tex_coord, sampler2D normal_map)
{
    // Create TBN matrix.
    mat3 TBN = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

    // Sample tangent space normal vector from normal map and remap it from [0, 1] to [-1, 1] range.
    vec3 n = texture(normal_map, tex_coord).xyz;

    n.y = 1.0 - n.y;

    n = normalize(n * 2.0 - 1.0);

    // Multiple vector by the TBN matrix to transform the normal from tangent space to world space.
    n = normalize(TBN * n);

    return n;
}

// ------------------------------------------------------------------------

vec3 F_schlick(in vec3 f0, in float vdoth)
{
    return f0 + (vec3(1.0) - f0) * (pow(1.0 - vdoth, 5.0));
}

// ------------------------------------------------------------------------

float D_ggx(in float ndoth, in float alpha)
{
    float a2    = alpha * alpha;
    float denom = (ndoth * ndoth) * (a2 - 1.0) + 1.0;

    return a2 / max(EPSILON, (M_PI * denom * denom));
}

// ------------------------------------------------------------------------

float G1_schlick_ggx(in float roughness, in float ndotv)
{
    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;

    return ndotv / max(EPSILON, (ndotv * (1.0 - k) + k));
}

// ------------------------------------------------------------------------

float G_schlick_ggx(in float ndotl, in float ndotv, in float roughness)
{
    return G1_schlick_ggx(roughness, ndotl) * G1_schlick_ggx(roughness, ndotv);
}

// ------------------------------------------------------------------------

vec3 evaluate_specular_brdf(in float roughness, in vec3 F, in float ndoth, in float ndotl, in float ndotv)
{
    float alpha = roughness * roughness;
    return (D_ggx(ndoth, alpha) * F * G_schlick_ggx(ndotl, ndotv, roughness)) / max(EPSILON, (4.0 * ndotl * ndotv));
}

// ------------------------------------------------------------------------

vec3 evaluate_diffuse_brdf(in vec3 diffuse_color)
{
    return diffuse_color / M_PI;
}

// ------------------------------------------------------------------------

vec3 evaluate_uber_brdf(in vec3 diffuse_color, in float roughness, in vec3 N, in vec3 F0, in vec3 Wo, in vec3 Wh, in vec3 Wi)
{
    float NdotL = max(dot(N, Wi), 0.0);
    float NdotV = max(dot(N, Wo), 0.0);
    float NdotH = max(dot(N, Wh), 0.0);
    float VdotH = max(dot(Wi, Wh), 0.0);

    vec3 F        = F_schlick(F0, VdotH);
    vec3 specular = evaluate_specular_brdf(roughness, F, NdotH, NdotL, NdotV);
    vec3 diffuse  = evaluate_diffuse_brdf(diffuse_color.xyz);

    return (vec3(1.0) - F) * diffuse + specular;
}

// ------------------------------------------------------------------------

vec3 direct_lighting(in vec3 Wo, in vec3 N, in vec3 P, in vec3 F0, in vec3 diffuse_color, in float roughness)
{
    vec3 Wi = -light_direction.xyz;
    vec3 Wh = normalize(Wo + Wi);

    vec3 brdf = evaluate_uber_brdf(diffuse_color, roughness, N, F0, Wo, Wh, Wi);

    vec3 Li = light_color.xyz;

    float NdotL = max(dot(N, Wi), 0.0);

    // Multiply the final result by NdotL
    return brdf * Li * NdotL * visibility(FS_IN_WorldPos);
}

// ------------------------------------------------------------------

vec3 add_inscattered_light(vec3 color, vec3 world_pos)
{
    vec3 uv = world_to_uv(world_pos, bias_near_far_pow.y, bias_near_far_pow.z, bias_near_far_pow.w, view_proj);

    vec4  scattered_light = u_Tricubic ? textureTricubic(s_VoxelGrid, uv) : textureLod(s_VoxelGrid, uv, 0.0f);
    float transmittance   = scattered_light.a;

    return color * transmittance + scattered_light.rgb;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    // Combine texture and factor for each property
    vec3  albedo    = texture(s_Albedo, FS_IN_TexCoord).rgb * u_AlbedoFactor;
    float roughness = texture(s_Roughness, FS_IN_TexCoord).r * u_RoughnessFactor;
    float metallic  = texture(s_Metallic, FS_IN_TexCoord).r * u_MetallicFactor;
    
    vec3  N  = get_normal_from_map(FS_IN_Tangent, FS_IN_Bitangent, FS_IN_Normal, FS_IN_TexCoord, s_Normal);
    vec3 Wo = normalize(camera_position.xyz - FS_IN_WorldPos);
    vec3 F0 = mix(vec3(0.04f), albedo, metallic);

    vec3 color = direct_lighting(Wo, N, FS_IN_WorldPos, F0, albedo, roughness);

    // Ambient
    color += albedo * light_color.w;

    // Volumetric Light & Tonemapping (unchanged)
    color = add_inscattered_light(color, FS_IN_WorldPos);
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FS_OUT_Color = vec4(color, 1.0);
}