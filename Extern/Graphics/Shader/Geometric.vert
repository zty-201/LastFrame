#version 310 es

precision highp float;
precision highp int;

layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexNormal;
layout(location=2) in vec2 VertexTexCoord;
layout(location=3) in vec4 VertexTangent;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out mat3 TBN;
out flat uint InstanceIndex;

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

layout(std430, binding=0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

void main() {

    InstanceIndex = uint(gl_InstanceID);
    InstanceData instance = instances[gl_InstanceID];
    
    // Same as your Basic.vert SCENE pass
    TexCoord = VertexTexCoord;
    mat4 MV = V * instance.modelMatrix; 
    mat3 N = mat3(vec3(MV[0]), vec3(MV[1]), vec3(MV[2])); 
    
    Normal = normalize(N * VertexNormal);

    vec3 T = normalize(N * VertexTangent.xyz);
    vec3 B = cross(Normal, T) * VertexTangent.w;
    TBN = mat3(T, B, Normal);

    vec4 VertexPositionInView = MV * vec4(VertexPosition, 1.0f);
    Position = VertexPositionInView.xyz;
    gl_Position = P * VertexPositionInView;
}