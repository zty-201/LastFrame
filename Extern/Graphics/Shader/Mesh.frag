#version 310 es
precision highp float;
precision highp int; 
precision highp sampler2D; 
precision highp sampler3D;
precision highp sampler2DArrayShadow;
precision highp samplerCubeShadow;

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
    mat4  inv_view_proj;
    mat4  cascade_view_proj[4];  // Array of cascade matrices
    vec4  cascade_splits;
    vec4  cascade_biases;
    vec4  light_direction;
    vec4  light_color;
    vec4  camera_position;
    vec4  bias_near_far_pow;
    vec4  aniso_density_scattering_absorption;
    vec4  time;
    ivec4 width_height;
};

struct PointLightData {
    vec4 position_radius;      
    vec4 color_intensity;      
    vec4 attenuation_type;     // .w is TYPE
    vec4 direction_shadow;     // .xyz is Dir, .w is ShadowIndex
    vec4 spot_cutoffs;         // .x=inner, .y=outer
};

layout(std430, binding = 1) readonly buffer PointLightBuffer {
    PointLightData lights[];
} pointLightBuffer;

uniform sampler2D s_Albedo;
uniform sampler2D s_Normal;
uniform sampler2D s_Roughness;
uniform sampler2D s_Metallic;   

uniform vec3  u_AlbedoFactor;
uniform float u_RoughnessFactor;
uniform float u_MetallicFactor;

uniform sampler2DArrayShadow s_CascadeShadowMaps;;
uniform sampler3D            s_VoxelGrid;

uniform bool u_Tricubic;
uniform int u_DebugMode;
uniform int u_NumPointLights;
uniform samplerCubeShadow s_PointShadowCubemaps[4]; 

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float sample_shadow_map(vec2 coord, float u, float v, float z, int cascade, float inv_shadow_map_size)
{
    vec2 uv = coord + vec2(u, v) * inv_shadow_map_size;
    
    // Scale bias per cascade
    float bias = bias_near_far_pow.x * (1.0 + float(cascade) * 0.5);
    
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
    
    if (any(greaterThan(proj_coords.xy, vec2(1.0f))) || any(lessThan(proj_coords.xy, vec2(0.0f))))
        return 1.0f;

    float shadow_map_size     = 4096.0f;
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
    
    // Select cascade
    int cascade = 3;
    if (depth < cascade_splits.x) cascade = 0;
    else if (depth < cascade_splits.y) cascade = 1;
    else if (depth < cascade_splits.z) cascade = 2;
    
    // Sample primary cascade
    float shadow = visibility_cascade(p, cascade);
    
    // Blend cascades
    float blend = getCascadeBlend(depth, cascade);
    if (blend > 0.0 && cascade < 3) {
        float shadow2 = visibility_cascade(p, cascade + 1);
        shadow = mix(shadow, shadow2, blend);
    }

    // Fade out shadows at the max shadow distance
    float maxShadowDist = cascade_splits.w;
    float fadeStart = maxShadowDist * 0.9;
    float distBlend = smoothstep(maxShadowDist, fadeStart, depth); // 1.0 close, 0.0 at max dist
    
    // If beyond max distance, return 0.0 (fully dark) to match out-of-bounds logic,
    // or we can clamp it to the last known shadow value.
    // Standard approach: Fade to 0.0 (dark) implies no shadow caster logic known.

    return mix(0.0, shadow, distBlend);
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
    vec4 coords = vec4(lightToFrag, currentDepth - bias);

    // Dynamic indexing of samplers is often forbidden in GLSL.
    // We use a manual switch.
    float shadow = 1.0;
    switch (shadowIndex) {
        case 0: shadow = texture(s_PointShadowCubemaps[0], coords); break;
        case 1: shadow = texture(s_PointShadowCubemaps[1], coords); break;
        case 2: shadow = texture(s_PointShadowCubemaps[2], coords); break;
        case 3: shadow = texture(s_PointShadowCubemaps[3], coords); break;
    }
    return shadow;
}

// ------------------------------------------------------------------

vec3 calculatePointLight(int lightIndex, vec3 worldPos, vec3 N, vec3 Wo, vec3 F0, vec3 diffuse_color, float roughness) {
    PointLightData light = pointLightBuffer.lights[lightIndex];
    
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
        shadow = getPointShadow(worldPos, light.position_radius.xyz, light.position_radius.w, idx);
    }
    
    // Combine everything, including spotIntensity
    return brdf * light.color_intensity.rgb * attenuation * shadow * NdotL * spotIntensity;
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
    
    // Clamp to safe ranges
    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic, 0.0, 1.0);
    
    vec3 N  = get_normal_from_map(FS_IN_Tangent, FS_IN_Bitangent, FS_IN_Normal, FS_IN_TexCoord, s_Normal);
    vec3 Wo = normalize(camera_position.xyz - FS_IN_WorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse_color = albedo * (1.0 - metallic);

    // ----------------------------DEBUGGING-------------------------------
    if (u_DebugMode == 1) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        int cascade = 3;
        if (depth < cascade_splits.x) cascade = 0;
        else if (depth < cascade_splits.y) cascade = 1;
        else if (depth < cascade_splits.z) cascade = 2;
        
        vec3 cascadeColors[4] = vec3[4](
            vec3(1.0, 0.0, 0.0),  // Red
            vec3(0.0, 1.0, 0.0),  // Green
            vec3(0.0, 0.0, 1.0),  // Blue
            vec3(1.0, 1.0, 0.0)   // Yellow
        );
        FS_OUT_Color = vec4(cascadeColors[cascade], 1.0);
        return;
    }
    
    // DEBUG MODE 2: Show shadow map UV coordinates
    if (u_DebugMode == 2) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        int cascade = 3;
        if (depth < cascade_splits.x) cascade = 0;
        else if (depth < cascade_splits.y) cascade = 1;
        else if (depth < cascade_splits.z) cascade = 2;
        
        vec4 light_space_pos = cascade_view_proj[cascade] * vec4(FS_IN_WorldPos, 1.0);
        vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
        proj_coords = proj_coords * 0.5 + 0.5;
        
        // Red = U, Green = V, Blue = 0 if in bounds, 1 if out
        float outOfBounds = (proj_coords.x < 0.0 || proj_coords.x > 1.0 || 
                            proj_coords.y < 0.0 || proj_coords.y > 1.0) ? 1.0 : 0.0;
        FS_OUT_Color = vec4(proj_coords.x, proj_coords.y, outOfBounds, 1.0);
        return;
    }
    
    // DEBUG MODE 3: Show raw shadow value (no PCF)
    if (u_DebugMode == 3) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        int cascade = 3;
        if (depth < cascade_splits.x) cascade = 0;
        else if (depth < cascade_splits.y) cascade = 1;
        else if (depth < cascade_splits.z) cascade = 2;
        
        vec4 light_space_pos = cascade_view_proj[cascade] * vec4(FS_IN_WorldPos, 1.0);
        vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
        proj_coords = proj_coords * 0.5 + 0.5;
        
        float bias = cascade_biases[cascade];
        float shadow = texture(s_CascadeShadowMaps, 
                               vec4(proj_coords.xy, float(cascade), proj_coords.z - bias));
        
        // White = lit, Black = shadowed
        FS_OUT_Color = vec4(vec3(shadow), 1.0);
        return;
    }
    
    // DEBUG MODE 4: Show depth comparison values
    if (u_DebugMode == 4) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        int cascade = 3;
        if (depth < cascade_splits.x) cascade = 0;
        else if (depth < cascade_splits.y) cascade = 1;
        else if (depth < cascade_splits.z) cascade = 2;
        
        vec4 light_space_pos = cascade_view_proj[cascade] * vec4(FS_IN_WorldPos, 1.0);
        vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;
        proj_coords = proj_coords * 0.5 + 0.5;
        
        // Visualize the Z value (depth)
        FS_OUT_Color = vec4(vec3(proj_coords.z), 1.0);
        return;
    }
    
    // DEBUG MODE 5: Show cascade boundaries with gradient
    if (u_DebugMode == 5) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        // Show depth as gradient within each cascade
        float t = 0.0;
        vec3 color;
        
        if (depth < cascade_splits.x) {
            t = depth / cascade_splits.x;
            color = mix(vec3(1,0,0), vec3(0.5,0,0), t);  // Red gradient
        }
        else if (depth < cascade_splits.y) {
            t = (depth - cascade_splits.x) / (cascade_splits.y - cascade_splits.x);
            color = mix(vec3(0,1,0), vec3(0,0.5,0), t);  // Green gradient
        }
        else if (depth < cascade_splits.z) {
            t = (depth - cascade_splits.y) / (cascade_splits.z - cascade_splits.y);
            color = mix(vec3(0,0,1), vec3(0,0,0.5), t);  // Blue gradient
        }
        else {
            t = (depth - cascade_splits.z) / (cascade_splits.w - cascade_splits.z);
            color = mix(vec3(1,1,0), vec3(0.5,0.5,0), t);  // Yellow gradient
        }
        
        FS_OUT_Color = vec4(color, 1.0);
        return;
    }
    
    // DEBUG MODE 6: Show cascade 0 only with shadow
    if (u_DebugMode == 6) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        if (depth < cascade_splits.x) {  // Only show cascade 0
            float shadow = visibility_cascade(FS_IN_WorldPos, 0);
            // Red tinted by shadow
            FS_OUT_Color = vec4(shadow, shadow * 0.2, shadow * 0.2, 1.0);
        } else {
            // Gray for other cascades
            FS_OUT_Color = vec4(0.5, 0.5, 0.5, 1.0);
        }
        return;
    }
    
    // DEBUG MODE 7: Show light direction alignment
    if (u_DebugMode == 7) {
        vec3 N = get_normal_from_map(FS_IN_Tangent, FS_IN_Bitangent, FS_IN_Normal, FS_IN_TexCoord, s_Normal);
        float NdotL = dot(N, -light_direction.xyz);
        
        // Red = facing away from light, Green = facing toward light
        vec3 color = NdotL > 0.0 ? vec3(0, NdotL, 0) : vec3(-NdotL, 0, 0);
        FS_OUT_Color = vec4(color, 1.0);
        return;
    }
    
    // DEBUG MODE 8: Show distance from camera
    if (u_DebugMode == 8) {
        float dist = length(camera_position.xyz - FS_IN_WorldPos);
        float normalized = dist / 50.0;  // Normalize to 50 units
        
        // Rainbow gradient based on distance
        vec3 color = vec3(normalized, 1.0 - normalized, sin(normalized * 3.14159));
        FS_OUT_Color = vec4(color, 1.0);
        return;
    }
    
    // DEBUG MODE 9: Show if in cascade 0 AND lit/shadowed
    if (u_DebugMode == 9) {
        vec4 view_pos = view * vec4(FS_IN_WorldPos, 1.0);
        float depth = abs(view_pos.z);
        
        if (depth < cascade_splits.x) {
            // In cascade 0
            float shadow = visibility(FS_IN_WorldPos);
            if (shadow > 0.5) {
                // Lit area in cascade 0 - BRIGHT GREEN (this is the "light block")
                FS_OUT_Color = vec4(0.0, 1.0, 0.0, 1.0);
            } else {
                // Shadowed area in cascade 0 - DARK RED
                FS_OUT_Color = vec4(0.5, 0.0, 0.0, 1.0);
            }
        } else {
            // Not in cascade 0 - render normally but dimmed
            vec3 color = direct_lighting(Wo, N, FS_IN_WorldPos, F0, diffuse_color, roughness);
            FS_OUT_Color = vec4(color * 0.3, 1.0);
        }
        return;
    }
    // -------------------------------------------------------------------------
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
    color = add_inscattered_light(color, FS_IN_WorldPos);

    // Tone mapping
    vec3 toneMapped = ((color * (0.15 * color + 0.05) + 0.004) / 
                   (color * (0.15 * color + 0.50) + 0.06)) - 0.02 / 0.30;
    color = toneMapped;
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FS_OUT_Color = vec4(color, 1.0);

}