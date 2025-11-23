#version 310 es
precision highp float;
precision highp samplerCube;
precision highp sampler3D;
// ------------------------------------------------------------------
// DEFINES ----------------------------------------------------------
// ------------------------------------------------------------------

#define PI 3.14159265359
#define PLANET_RADIUS 6371000.0
#define ATMOSPHERE_RADIUS 6471000.0
#define NUM_SAMPLES 16
const vec3 betaRay = vec3(5.5e-6, 13.0e-6, 22.4e-6); // Rayleigh (Blue sky)
const vec3 betaMie = vec3(21.0e-6);                  // Mie (Haze/Fog)

// ------------------------------------------------------------------
// INPUTS  ----------------------------------------------------------
// ------------------------------------------------------------------

layout(location=0) out vec4 FS_OUT_Color;

// ------------------------------------------------------------------
// OUTPUTS  ---------------------------------------------------------
// ------------------------------------------------------------------

in vec3 FS_IN_WorldPos;

// ------------------------------------------------------------------
// UNIFORMS  --------------------------------------------------------
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

uniform samplerCube s_Cubemap;
uniform sampler3D s_VoxelGrid;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec2 raySphereIntersect(vec3 ro, vec3 rd, float radius) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float h = b * b - c;
    if (h < 0.0) return vec2(-1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 viewDir = normalize(FS_IN_WorldPos); 

    vec3 sunDir = -normalize(light_direction.xyz);

    // We simulate the camera being on the planet surface
    // If your game world is small, we add the planet radius to the Y position
    vec3 camPos = vec3(0.0, PLANET_RADIUS + 1000.0, 0.0); 
    // OR: vec3 camPos = vec3(camera_position.x, camera_position.y + PLANET_RADIUS, camera_position.z);

    // Check intersection with atmosphere
    vec2 hit = raySphereIntersect(camPos, viewDir, ATMOSPHERE_RADIUS);
    if (hit.y < 0.0) {
        FS_OUT_Color = vec4(0.0, 0.0, 0.0, 1.0); // Space/Void
        return;
    }

    float distToAtmosphere = hit.y;
    float stepSize = distToAtmosphere / float(NUM_SAMPLES);
    
    vec3 totalRay = vec3(0.0);
    vec3 totalMie = vec3(0.0);
    
    float opticalDepthRay = 0.0;
    float opticalDepthMie = 0.0;
    
    // Phase functions
    float mu = dot(viewDir, sunDir);
    float phaseRay = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
    float g = aniso_density_scattering_absorption.x; // Use your fog anisotropy
    float phaseMie = 3.0 / (8.0 * PI) * ((1.0 - g * g) * (1.0 + mu * mu)) / ((2.0 + g * g) * pow(1.0 + g * g - 2.0 * g * mu, 1.5));

    vec3 currentPos = camPos;
    
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        vec3 newPos = currentPos + viewDir * stepSize;
        float height = length(currentPos) - PLANET_RADIUS;
        
        // Scale height densities (8km for Ray, 1.2km for Mie)
        float hr = exp(-height / 8000.0) * stepSize;
        float hm = exp(-height / 1200.0) * stepSize;
        
        opticalDepthRay += hr;
        opticalDepthMie += hm;
        
        // Secondary ray to sun (light absorption)
        vec2 sunHit = raySphereIntersect(currentPos, sunDir, ATMOSPHERE_RADIUS);
        float sunStep = sunHit.y / 8.0; // fewer samples for secondary ray
        float sunOpticalRay = 0.0;
        float sunOpticalMie = 0.0;
        
        vec3 sunPos = currentPos;
        for(int j = 0; j < 4; ++j) { // Fast 4 sample secondary ray
            vec3 sunSamplePos = sunPos + sunDir * sunStep;
            float sunHeight = length(sunSamplePos) - PLANET_RADIUS;
            sunOpticalRay += exp(-sunHeight / 8000.0) * sunStep;
            sunOpticalMie += exp(-sunHeight / 1200.0) * sunStep;
            sunPos = sunSamplePos;
        }
        
        vec3 attenuation = exp(-(betaRay * (opticalDepthRay + sunOpticalRay) + betaMie * (opticalDepthMie + sunOpticalMie)));
        
        totalRay += attenuation * hr;
        totalMie += attenuation * hm;
        
        currentPos = newPos;
    }
    
    vec3 color = (totalRay * betaRay * phaseRay + totalMie * betaMie * phaseMie) * light_color.xyz; // * Sun Intensity

    // Simple HDR Tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FS_OUT_Color = vec4(color, 1.0);
}