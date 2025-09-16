#version 420 core

layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexNormal;
layout(location=2) in vec2 VertexTexCoord;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Vec;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform int Pass; // Pass number

void main()
{
    if (Pass == 0) {

        Vec = VertexPosition; 
        gl_Position = P * vec4(vec3(M * V * vec4(VertexPosition, 0.f)).xyz, 1.f);

    } else {
        // Scene pass
        TexCoord = VertexTexCoord;

        mat4 MV = V * M; 
        mat3 N = mat3(vec3(MV[0]), vec3(MV[1]), vec3(MV[2])); 

        Normal = normalize(N * VertexNormal);

        vec4 VertexPositionInView = MV * vec4(VertexPosition, 1.0f);
        Position = VertexPositionInView.xyz;
        gl_Position = P * VertexPositionInView; 
    }
}


