#version 320 es
precision highp float;
precision highp int;
precision highp sampler2D;
precision highp sampler3D;

#include <common.glsl>

#define M_PI 3.14159265359
#define EPSILON 0.0001f

layout(location=0) out vec4 FS_OUT_Color;

in vec3 FS_IN_WorldPos;
in vec3 FS_IN_Normal;
in vec2 FS_IN_TexCoord;
in vec3 FS_IN_Tangent;
in vec3 FS_IN_Bitangent;

layout(std140, binding = 0) uniform VolumetricUBO
{
    mat4  view;
    mat4  projection;
    mat4  view_proj;
    mat4  prev_view_proj;
    mat4  inv_view_proj;
    mat4  cascade_view_proj[3];
    vec4  cascade_splits;
    vec4  cascade_biases;
    vec4  light_direction;
    vec4  light_color;
    vec4  camera_position;
    vec4  bias_near_far_pow;
    vec4  aniso_density_scattering_absorption;
    vec4  width_height_time;
};

struct LightSSBO {
    vec4 position_radius;
    vec4 color_intensity;
    vec4 attenuation_type;
    vec4 direction_shadow;
    vec4 spot_cutoffs;
    mat4 view_proj;
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    LightSSBO lights[];
} lightBuffer;

uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;
uniform sampler2D s_Roughness;
uniform sampler2D s_Metallic;

uniform vec3  u_AlbedoFactor;
uniform float u_RoughnessFactor;
uniform float u_MetallicFactor;
uniform bool  u_AndroidPlatform;
uniform int   u_NumPointLights;

vec3 apply_analytical_fog(vec3 color, vec3 world_pos, vec3 camera_pos)
{
    float density = aniso_density_scattering_absorption.y / 1000.0f;

    float dist = length(world_pos - camera_pos);
    float fogAmount  = density * dist;
    float visibility = clamp(exp(-fogAmount), 0.0, 1.0);

    vec3 fogColor = vec3(0.5, 0.7, 1.0) * light_color.w;

    return mix(fogColor, color, visibility);
}

vec3 get_normal_from_map(vec3 tangent, vec3 bitangent, vec3 normal, vec2 tex_coord, sampler2D normal_map)
{
    mat3 TBN = mat3(normalize(tangent), normalize(bitangent), normalize(normal));

    vec3 n = texture(normal_map, tex_coord).xyz;
    n.y = 1.0 - n.y;
    n = normalize(n * 2.0 - 1.0);

    return normalize(TBN * n);
}

vec3 F_schlick(in vec3 f0, in float vdoth)
{
    return f0 + (vec3(1.0) - f0) * pow(1.0 - vdoth, 5.0);
}

float D_ggx(in float ndoth, in float alpha)
{
    float a2    = alpha * alpha;
    float denom = (ndoth * ndoth) * (a2 - 1.0) + 1.0;
    return a2 / max(EPSILON, (M_PI * denom * denom));
}

float G1_schlick_ggx(in float roughness, in float ndotv)
{
    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    return ndotv / max(EPSILON, (ndotv * (1.0 - k) + k));
}

float G_schlick_ggx(in float ndotl, in float ndotv, in float roughness)
{
    return G1_schlick_ggx(roughness, ndotl) * G1_schlick_ggx(roughness, ndotv);
}

vec3 evaluate_specular_brdf(in float roughness, in vec3 F, in float ndoth, in float ndotl, in float ndotv)
{
    float alpha = roughness * roughness;
    return (D_ggx(ndoth, alpha) * F * G_schlick_ggx(ndotl, ndotv, roughness)) / max(EPSILON, (4.0 * ndotl * ndotv));
}

vec3 evaluate_diffuse_brdf(in vec3 diffuse_color)
{
    return diffuse_color / M_PI;
}

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

vec3 direct_lighting(in vec3 Wo, in vec3 N, in vec3 F0, in vec3 diffuse_color, in float roughness)
{
    vec3 Wi = normalize(-light_direction.xyz);
    vec3 Wh = normalize(Wo + Wi);
    float NdotL = max(dot(N, Wi), 0.0);

    vec3 brdf = evaluate_uber_brdf(diffuse_color, roughness, N, F0, Wo, Wh, Wi);
    vec3 Li = light_color.xyz;

    return brdf * Li * NdotL;
}

vec3 calculatePointLightNoShadow(int lightIndex, vec3 worldPos, vec3 N, vec3 Wo, vec3 F0, vec3 diffuse_color, float roughness)
{
    LightSSBO light = lightBuffer.lights[lightIndex];

    vec3 lightVec = light.position_radius.xyz - worldPos;
    float dist = length(lightVec);

    if (dist > light.position_radius.w) return vec3(0.0);

    vec3 Wi = normalize(lightVec);

    float spotIntensity = 1.0;

    if (light.attenuation_type.w > 1.5)
    {
        float theta = dot(normalize(-Wi), normalize(light.direction_shadow.xyz));
        float epsilon = light.spot_cutoffs.x - light.spot_cutoffs.y;
        spotIntensity = clamp((theta - light.spot_cutoffs.y) / max(epsilon, 0.0001), 0.0, 1.0);
    }

    vec3 Wh = normalize(Wo + Wi);
    float NdotL = max(dot(N, Wi), 0.0);

    float attenuationDivisor =
        light.attenuation_type.x +
        light.attenuation_type.y * dist +
        light.attenuation_type.z * dist * dist;

    float attenuation = 1.0 / max(attenuationDivisor, 0.0001);

    vec3 brdf = evaluate_uber_brdf(diffuse_color, roughness, N, F0, Wo, Wh, Wi);

    return brdf * light.color_intensity.rgb * attenuation * NdotL * spotIntensity;
}

void main()
{
    vec4 albedoTex = texture(s_Albedo, FS_IN_TexCoord);

    if (albedoTex.a < 0.5)
    {
        discard;
    }

    vec3  albedo    = albedoTex.rgb * u_AlbedoFactor;
    float roughness = texture(s_Roughness, FS_IN_TexCoord).r * u_RoughnessFactor;
    float metallic  = texture(s_Metallic, FS_IN_TexCoord).r * u_MetallicFactor;

    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic, 0.0, 1.0);

    vec3 N  = get_normal_from_map(FS_IN_Tangent, FS_IN_Bitangent, FS_IN_Normal, FS_IN_TexCoord, s_Normal);
    vec3 Wo = normalize(camera_position.xyz - FS_IN_WorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse_color = albedo * (1.0 - metallic);

    vec3 skyColor = vec3(0.5, 0.7, 1.0);
    vec3 groundColor = vec3(0.2, 0.2, 0.2);
    float hemiFactor = N.y * 0.5 + 0.5;
    vec3 ambientLight = mix(groundColor, skyColor, hemiFactor) * max(light_color.w, 0.0);

    vec3 color = albedo * ambientLight;
    color += direct_lighting(Wo, N, F0, diffuse_color, roughness);

    for (int i = 0; i < u_NumPointLights; ++i)
    {
        color += calculatePointLightNoShadow(i, FS_IN_WorldPos, N, Wo, F0, diffuse_color, roughness);
    }

    if (u_AndroidPlatform)
    {
        color = apply_analytical_fog(color, FS_IN_WorldPos, camera_position.xyz);
    }

    vec3 toneMapped = ((color * (0.15 * color + 0.05) + 0.004) /
                       (color * (0.15 * color + 0.50) + 0.06)) - 0.02 / 0.30;

    color = toneMapped;
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

    FS_OUT_Color = vec4(color, 1.0);
}