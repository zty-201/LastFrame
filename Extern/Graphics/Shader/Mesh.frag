#version 320 es

// 1. The Global 16-bit Override
precision mediump float;
precision mediump int; 
precision mediump sampler2D; 

// 2. Protect 3D and Depth Samplers
precision highp sampler3D;
precision highp sampler2DArrayShadow;
precision highp samplerCubeArrayShadow;

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

in highp vec3 FS_IN_WorldPos;   
in highp vec2 FS_IN_TexCoord;   
in mediump vec3 FS_IN_Normal;   
in mediump vec3 FS_IN_Tangent;
in mediump vec3 FS_IN_Bitangent;

//in flat float FS_IN_Roughness;
//in flat float FS_IN_Metallic;

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

struct LightSSBO {
    vec4 position_radius;      
    vec4 color_intensity;      
    vec4 attenuation_type;     // .w is TYPE
    vec4 direction_shadow;     // .xyz is Dir, .w is ShadowIndex
    vec4 spot_cutoffs;         // .x=inner, .y=outer
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

uniform sampler2DArrayShadow s_CascadeShadowMaps;
uniform sampler2DArrayShadow s_SpotShadowMaps;
uniform sampler3D            s_VoxelGrid;

uniform bool u_Tricubic;
uniform bool u_AndroidPlatform;
uniform int u_NumPointLights;
uniform samplerCubeArrayShadow s_PointShadowArray;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec3 apply_analytical_fog(vec3 color, vec3 world_pos, vec3 camera_pos) 
{
    float density = clamp(aniso_density_scattering_absorption.y / 1000.0, 0.0, 1.0);
    float dist = length(world_pos - camera_pos);

    float fogAmount  = density * dist;
    
    // FIX 1: Rename to fogFactor to avoid function name collision!
    // FIX 2: Add 0.0001 to prevent exp(-0.0) NaN driver bugs
    float fogFactor = clamp(exp(-max(fogAmount, 0.0001)), 0.0, 1.0);
    
    vec3 fogColor = vec3(0.5, 0.7, 1.0) * light_color.w;

    return mix(fogColor, color, fogFactor);
}

// ------------------------------------------------------------------

float getSpotLightVisibility(vec3 worldPos, mat4 lightMatrix, int shadowIndex) {

    // 2. Project
    vec4 lightSpacePos = lightMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5; // [-1,1] -> [0,1]

    // 3. Bounds Check
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    // 4. Sample
    float bias = 0.0005; 
    return texture(s_SpotShadowMaps, vec4(projCoords.xy, float(shadowIndex), projCoords.z - bias));
}

// ------------------------------------------------------------------

float sample_shadow_map(vec2 coord, float u, float v, float z, int cascade, float inv_shadow_map_size)
{
    vec2 uv = coord + vec2(u, v) * inv_shadow_map_size;
    
    // Decrease the bias for larger cascades because their Z-range is massive.
    // Using powers of 2 (1.0, 2.0, 4.0) roughly matches frustum volume growth.
    float cascade_scale = pow(2.0, float(cascade)); 
    float bias = bias_near_far_pow.x / cascade_scale;
    
    return texture(s_CascadeShadowMaps, vec4(uv, float(cascade), z - bias));
}

// ------------------------------------------------------------------

float getCascadeBlend(float depth, int cascade) {
    // No blending for the last cascade
    if (cascade >= 3) return 0.0;
    
    // Get the split distance for this cascade
    float splitDist = cascade == 0 ? cascade_splits.x :
                     cascade == 1 ? cascade_splits.y :
                     cascade == 2 ? cascade_splits.z : cascade_splits.w;
    
    // Define blend region (10% of the cascade range)
    float blendRange = splitDist * 0.1;
    
    // Calculate blend factor (0 = fully this cascade, 1 = fully next cascade)
    return smoothstep(splitDist - blendRange, splitDist, depth);
}

// ------------------------------------------------------------------

float visibility_cascade(vec3 p, int cascade)
{
    // Use the appropriate cascade matrix
    vec4 light_space_pos = cascade_view_proj[cascade] * vec4(p, 1.0);
    vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
    proj_coords = proj_coords * 0.5 + 0.5;
    
    if (any(greaterThan(proj_coords.xy, vec2(1.0f))) || any(lessThan(proj_coords.xy, vec2(0.0f))) || proj_coords.z > 1.0f)
        return 1.0f;

    float shadow_map_size     = width_height_time.x;
    float inv_shadow_map_size = 1.0f / shadow_map_size;

    if (u_AndroidPlatform){
        float bias = bias_near_far_pow.x / pow(2.0f, float(cascade));
        float shadow = 0.0f;
        shadow += texture(s_CascadeShadowMaps, vec4(proj_coords.xy + vec2(-0.94, 0.34) * inv_shadow_map_size, float(cascade), proj_coords.z - bias));
        shadow += texture(s_CascadeShadowMaps, vec4(proj_coords.xy + vec2( 0.94,-0.34) * inv_shadow_map_size, float(cascade), proj_coords.z - bias));
        shadow += texture(s_CascadeShadowMaps, vec4(proj_coords.xy + vec2(-0.09,-0.94) * inv_shadow_map_size, float(cascade), proj_coords.z - bias));
        shadow += texture(s_CascadeShadowMaps, vec4(proj_coords.xy + vec2( 0.34, 0.94) * inv_shadow_map_size, float(cascade), proj_coords.z - bias));
        return shadow * 0.25f;

    }
    
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

    sum += uw0 * vw0 * sample_shadow_map(base_uv, u0, v0, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw1 * vw0 * sample_shadow_map(base_uv, u1, v0, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw2 * vw0 * sample_shadow_map(base_uv, u2, v0, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw3 * vw0 * sample_shadow_map(base_uv, u3, v0, proj_coords.z, cascade, inv_shadow_map_size);

    sum += uw0 * vw1 * sample_shadow_map(base_uv, u0, v1, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw1 * vw1 * sample_shadow_map(base_uv, u1, v1, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw2 * vw1 * sample_shadow_map(base_uv, u2, v1, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw3 * vw1 * sample_shadow_map(base_uv, u3, v1, proj_coords.z, cascade, inv_shadow_map_size);

    sum += uw0 * vw2 * sample_shadow_map(base_uv, u0, v2, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw1 * vw2 * sample_shadow_map(base_uv, u1, v2, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw2 * vw2 * sample_shadow_map(base_uv, u2, v2, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw3 * vw2 * sample_shadow_map(base_uv, u3, v2, proj_coords.z, cascade, inv_shadow_map_size);

    sum += uw0 * vw3 * sample_shadow_map(base_uv, u0, v3, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw1 * vw3 * sample_shadow_map(base_uv, u1, v3, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw2 * vw3 * sample_shadow_map(base_uv, u2, v3, proj_coords.z, cascade, inv_shadow_map_size);
    sum += uw3 * vw3 * sample_shadow_map(base_uv, u3, v3, proj_coords.z, cascade, inv_shadow_map_size);

    return sum * 1.0f / 2704.0f;
}

// ------------------------------------------------------------------

float visibility(vec3 p)
{
    // Get view space depth for cascade selection
    vec4 view_pos = view * vec4(p, 1.0);
    float depth = abs(view_pos.z);
    
    // Select cascade. Default to 2 (the last valid cascade) instead of 3.
    int cascade = 2; 
    if (depth < cascade_splits.x) cascade = 0;
    else if (depth < cascade_splits.y) cascade = 1;
    
    // Sample primary cascade
    float shadow = visibility_cascade(p, cascade);

    // Blend cascades (only blend if we aren't on the last cascade)
    float blend = getCascadeBlend(depth, cascade);
    if (blend > 0.0 && cascade < 2) { 
        float shadow2 = visibility_cascade(p, cascade + 1);
        shadow = mix(shadow, shadow2, blend);
    }

    // Fix: Use .z as the max distance, because C++ passed 0.0 to .w
    float maxShadowDist = cascade_splits.z; 
    float fadeStart = maxShadowDist * 0.9;
    
    // Fix: smoothstep requires edge0 < edge1
    float fade = smoothstep(fadeStart, maxShadowDist, depth);
    
    // Fix: Fade to 1.0 (fully lit) in the distance, not 0.0 (dark)
    return mix(shadow, 1.0, fade); 
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
    float NdotL = max(dot(N, Wi), 0.0); // Calculate Dot Product

    vec3 brdf = evaluate_uber_brdf(diffuse_color, roughness, N, F0, Wo, Wh, Wi);
    vec3 Li = light_color.xyz;

    float shadow_val = visibility(FS_IN_WorldPos);
    
    return brdf * Li * shadow_val * NdotL;
}

// ------------------------------------------------------------------

vec3 add_inscattered_light(vec3 color, vec3 world_pos)
{
    vec3 uv = world_to_uv(world_pos, bias_near_far_pow.y, bias_near_far_pow.z, bias_near_far_pow.w, view_proj);

    vec4  scattered_light = u_Tricubic ? textureTricubic(s_VoxelGrid, uv) : texture(s_VoxelGrid, uv);
    float transmittance   = scattered_light.a;

    return color * transmittance + scattered_light.rgb;
}

// ------------------------------------------------------------------

float getPointShadow(vec3 worldPos, vec3 lightPos, float farPlane, int shadowIndex) {
    vec3 lightToFrag = worldPos - lightPos;
    float currentDepth = length(lightToFrag) / farPlane;
    float bias = 0.005;
    
    // Construct the 4D coordinate:
    // .xyz = Direction vector
    // .w   = Layer Index (The light index)
    vec4 coords = vec4(lightToFrag, float(shadowIndex));
    
    // Sample with comparison (refZ)
    // texture(sampler, vec4(dir, layer), refZ)
    return texture(s_PointShadowArray, coords, currentDepth - bias);
}

// ------------------------------------------------------------------

vec3 calculatePointLight(int lightIndex, vec3 worldPos, vec3 N, vec3 Wo, vec3 F0, vec3 diffuse_color, float roughness) {
    LightSSBO light = lightBuffer.lights[lightIndex];
    
    vec3 lightVec = light.position_radius.xyz - worldPos;
    float dist = length(lightVec);
    
    if (dist > light.position_radius.w) return vec3(0.0);
    
    vec3 Wi = normalize(lightVec);
    
    // --- NEW: Spotlight Intensity Logic ---
    float spotIntensity = 1.0;
    
    // Check if Type == 2.0 (Spot)
    if (light.attenuation_type.w > 1.5) { 
        // Dot product of Light Direction and Vector from Light to Pixel
        // Note: Wi is Pixel->Light. We need Light->Pixel, which is -Wi.
        // light.direction_shadow.xyz should be normalized!
        float theta = dot(normalize(-Wi), normalize(light.direction_shadow.xyz));
        
        float epsilon = light.spot_cutoffs.x - light.spot_cutoffs.y; // inner - outer
        spotIntensity = clamp((theta - light.spot_cutoffs.y) / epsilon, 0.0, 1.0);
    }
    // --------------------------------------

    vec3 Wh = normalize(Wo + Wi);
    float NdotL = max(dot(N, Wi), 0.0);
    
    // Standard Attenuation
    float attenuationDivisor = light.attenuation_type.x + 
                               light.attenuation_type.y * dist + 
                               light.attenuation_type.z * dist * dist;
    float attenuation = 1.0 / max(attenuationDivisor, 0.0001);
    
    vec3 brdf = evaluate_uber_brdf(diffuse_color, roughness, N, F0, Wo, Wh, Wi);
    
    // Shadow Logic (Using stored index at .w)
    float shadow = 1.0;
    if (light.direction_shadow.w >= 0.0) {

        int idx = int(round(light.direction_shadow.w));
        
        if (light.attenuation_type.w > 1.5) { 
            // IS SPOTLIGHT -> Sample 2D Array
            shadow = getSpotLightVisibility(worldPos, light.view_proj, idx);
        } 
        else {
            // IS POINT LIGHT -> Sample Cubemap
            shadow = getPointShadow(worldPos, light.position_radius.xyz, light.position_radius.w, idx);
        }
    }
    
    // Combine everything, including spotIntensity
    return brdf * light.color_intensity.rgb * attenuation * shadow * NdotL * spotIntensity;
}

vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{

    // Sample material properties
    vec3  albedo    = texture(s_Albedo, FS_IN_TexCoord).rgb * u_AlbedoFactor;
    float roughness = texture(s_Roughness, FS_IN_TexCoord).r * u_RoughnessFactor;
    float metallic  = texture(s_Metallic, FS_IN_TexCoord).r * u_MetallicFactor;
    float alpha     = texture(s_Albedo, FS_IN_TexCoord).a;

    if (alpha < 0.5){
        discard;
    }
    
    // Clamp to safe ranges
    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic, 0.0, 1.0);
    
    vec3 N  = get_normal_from_map(FS_IN_Tangent, FS_IN_Bitangent, FS_IN_Normal, FS_IN_TexCoord, s_Normal);
    vec3 Wo = normalize(camera_position.xyz - FS_IN_WorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse_color = albedo * (1.0 - metallic);

    // Direct lighting
    vec3 color = direct_lighting(Wo, N, FS_IN_WorldPos, F0, diffuse_color, roughness);
    
    // Point lights
    for (int i = 0; i < u_NumPointLights; i++) {
        color += calculatePointLight(i, FS_IN_WorldPos, N, Wo, F0, diffuse_color, roughness);
    }

    // Ambient
    vec3 skyColor = vec3(0.5, 0.7, 1.0); // Blueish Sky
    vec3 groundColor = vec3(0.2, 0.2, 0.2); // Dark Ground
    
    float hemiFactor = normalize(N).y * 0.5 + 0.5; // Map Y [-1, 1] to [0, 1]
    vec3 ambientLight = mix(groundColor, skyColor, hemiFactor) * light_color.w;
    
    color += diffuse_color * ambientLight;
    
    // Volumetric fog
    if (u_AndroidPlatform){
        color = apply_analytical_fog(color, FS_IN_WorldPos, camera_position.xyz);
    }
    else{
        color = add_inscattered_light(color, FS_IN_WorldPos);
    }

    // Tone mapping
    vec3 toneMapped = ((color * (0.15 * color + 0.05) + 0.004) / 
                   (color * (0.15 * color + 0.50) + 0.06)) - 0.02 / 0.30;
    color = toneMapped;
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FS_OUT_Color = vec4(color, 1.0);

}