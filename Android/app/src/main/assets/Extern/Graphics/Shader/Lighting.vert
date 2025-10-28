#version 310 es

precision highp float;
precision highp int;

layout(location=0) in vec2 Position;
layout(location=1) in vec2 TexCoords;

out vec2 TexCoord;

void main() {
    TexCoord = TexCoords;
    gl_Position = vec4(Position, 0.0, 1.0);
}