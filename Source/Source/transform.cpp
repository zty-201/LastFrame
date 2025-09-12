#include "pch.hpp"
#include "global.hpp"
#include "transform.h"

float* Pntr(mat4& M)
{
    return &(M[0][0]);
}

// Return a rotation matrix around an axis (0:X, 1:Y, 2:Z) by an angle
// measured in degrees.  NOTE: Make sure to convert degrees to radians
// before using sin and cos.  HINT: radians = degrees*PI/180
mat4 Rotate(const int i, const float theta)
{
    mat4 R = mat4(1.0f);  // Initialize to identity matrix!
    int j = (i+1) % 3;
    int k = (j+1) % 3;
    R[j][j] = R[k][k] = cos(theta*PI/180.0f);
    R[j][k] = sin(theta*PI/180.0f);
    R[k][j] = -R[j][k];
    return R;
}

// Return a scale matrix
mat4 Scale(vec3 s) { return Scale(s[0], s[1], s[2]); }
mat4 Scale(const float x, const float y, const float z)
{
    mat4 S = mat4(1.0f);  // Initialize to identity matrix!
    S[0][0] = x;
    S[1][1] = y;
    S[2][2] = z;
    return S;
}

// Return a translation matrix
mat4 Translate(vec3 s) { return Translate(s[0], s[1], s[2]); }
mat4 Translate(const float x, const float y, const float z)
{
    mat4 T = mat4(1.0f);  // Initialize to identity matrix!
    T[3][0] = x;
    T[3][1] = y;
    T[3][2] = z;
    return T;
}

// Returns a perspective projection matrix
mat4 Perspective(const float rx, const float ry,
             const float front, const float back)
{
    mat4 P = mat4(1.0f);  // Initialize to identity matrix!
    P[0][0] = 1.0f/rx;
    P[1][1] = 1.0f/ry;
    P[2][2] = -(back+front)/(back-front);
    P[3][2] = -(2.0f*front*back)/(back-front);
    P[2][3] = -1;
    P[3][3] = 0;
    return P;
}


