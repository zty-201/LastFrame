#version 310 es

//Specify precision for es. Use highp for more accuracy in 3D.
precision highp float;
precision highp int;

layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexNormal;
layout(location=2) in vec2 VertexTexCoord;
layout(location=3) in vec4 VertexTangent;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Vec;
out mat3 TBN;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform int Pass; // Pass number

void main()
{
    // Pass 0 is SCENE
    if (Pass == 0) {
        TexCoord = VertexTexCoord;
        mat4 MV = V * M; 
        mat3 N = mat3(vec3(MV[0]), vec3(MV[1]), vec3(MV[2])); 
        
        Normal = normalize(N * VertexNormal);

        vec3 T = normalize(N * VertexTangent.xyz);
        vec3 B = cross(Normal, T) * VertexTangent.w; // Use handedness
        TBN = mat3(T, B, Normal);

        vec4 VertexPositionInView = MV * vec4(VertexPosition, 1.0f);
        Position = VertexPositionInView.xyz;
        gl_Position = P * VertexPositionInView; 
    } 
    // Pass 1 is SKYBOX
    else {
        Vec = VertexPosition;
        mat4 viewNoTranslation = mat4(mat3(V));
        vec4 pos = P * viewNoTranslation * vec4(VertexPosition, 1.0);
        gl_Position = pos.xyzz;
    }
}