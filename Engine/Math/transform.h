////////////////////////////////////////////////////////////////////////
// A small library of 4x4 matrix operations needed for graphics
// transformations.  mat4 is a 4x4 float matrix class with indexing
// and printing methods.  A small list or procedures are supplied to
// create Rotate, Scale, Translate, and Perspective matrices and to
// return the product of any two such.

#ifndef _TRANSFORM_
#define _TRANSFORM_

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>          // For printing GLM objects with to_string
using namespace glm;

#include <fstream>

// Factory functions to create specific transformations, multiply two, and invert one.
mat4 Rotate(const int i, const float theta);
mat4 Scale(const float x, const float y, const float z);
mat4 Scale(vec3);
mat4 Translate(const float x, const float y, const float z);
mat4 Translate(vec3);
mat4 Perspective(const float rx, const float ry,
                 const float front, const float back);
mat4 LookAt(const vec3 Eye, const vec3 Center, const vec3 Up);

float* Pntr(mat4& m);

#endif
