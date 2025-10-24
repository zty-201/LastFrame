#version 310 es

precision highp float;
precision highp int;

in vec2 TexCoord;

layout(location=0) out vec4 FragColor;

// G-buffer inputs
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Reuse your existing structs
struct Light {
    vec3 position;
    vec3 L;
    vec3 La;
    vec3 Ld;
    vec3 Ls;
};

struct Material {
    float rough;
    float metal;
    vec3 color;
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
    float effect;          
};

struct Fog {
    float minDist;            
    float maxDist;            
    vec3 color;            
};

uniform Light light[1];
uniform Material material;
uniform Fog fog;
uniform mat4 V;
uniform int materialType;

const float PI = 3.14159265358979323846;

// Copy your lighting calculation functions from Basic.frag
float ggxDistribution(float nDotH, float roughness) {
    float alpha2 = roughness * roughness * roughness * roughness;
    float d = (nDotH * nDotH) * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * d * d);
}

float geomSmith(float nDotL, float roughness) {
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    float denom = nDotL * (1.0f - k) + k;
    return 1.0f / denom;
}

vec3 schlickFresnel(float lDotH, vec3 baseReflectivity) {
    return baseReflectivity + (1.0f - baseReflectivity) * pow(1.0f - lDotH, 5.0);
}

vec3 microfacetModel(vec3 position, vec3 normal, vec3 albedo, float metallic) {
    vec3 lightI = light[0].L;
    vec3 lightPositionInView = (V * vec4(light[0].position, 1.0f)).xyz;

    vec3 l = lightPositionInView - position;
    float dist = length(l);
    l = normalize(l);
    lightI *= 100.0 / (dist * dist);

    vec3 v = normalize(-position);
    vec3 h = normalize(v + l);
    float nDotH = dot(normal, h);
    float lDotH = dot(l, h);
    float nDotL = max(dot(normal, l), 0.0f);
    float nDotV = dot(normal, v);
    
    // Base reflectivity for metals vs dielectrics
    vec3 f0 = vec3(0.04); 
    if (metallic > 0.5) f0 = albedo;
    
    vec3 specBrdf = 0.25f * ggxDistribution(nDotH, material.rough) * 
                           schlickFresnel(lDotH, f0) * 
                           geomSmith(nDotL, material.rough) * 
                           geomSmith(nDotV, material.rough);

    return (albedo + PI * specBrdf) * lightI * nDotL;
}

vec3 BlinnPhong(vec3 position, vec3 normal, vec3 albedo) {
    vec3 lightPosView = vec3(V * vec4(light[0].position, 1.0f));

    vec3 L = lightPosView - position;
    if (length(L) < 1e-6f) return vec3(0.0f);
    L = normalize(L);

    vec3 V = normalize(-position);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(normal, L), 0.0f);
    float NdotH = max(dot(normal, H), 0.0f);

    vec3 ambient = light[0].La * material.Ka;
    vec3 diffuse = light[0].Ld * material.Kd * NdotL * albedo;
    vec3 specular = (NdotL > 0.0f) ? light[0].Ls * material.Ks * 
                    pow(NdotH, material.shininess) : vec3(0.0f);

    return ambient + diffuse + specular;
}

void main() {
    // 1. Retrieve data from G-buffer
    vec3 position = texture(gPosition, TexCoord).rgb;
    vec3 normal = texture(gNormal, TexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoord);
    vec3 albedo = albedoSpec.rgb;
    float metallic = albedoSpec.a;
    
    // 2. Calculate lighting based on material type
    vec3 color;
    if (materialType == 0) {
        color = microfacetModel(position, normal, albedo, metallic);
    } else {
        color = BlinnPhong(position, normal, albedo);
    }
    
    // 3. Apply fog (same as in your Basic.frag)
    float dist = length(position);
    float fogFactor = clamp((dist - fog.minDist) / (fog.maxDist - fog.minDist), 0.0f, 1.0f);
    color = mix(color, fog.color, fogFactor);
    
    // 4. Apply gamma correction
    color = pow(color, vec3(1.0f/2.2f));
    
    FragColor = vec4(color, 1.0);
}