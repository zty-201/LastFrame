////////////////////////////////////////////////////////////////////////
// A small library of object shapes (ground plane, sphere, and the
// famous Utah teapot), each created as a Vertex Array Object (VAO).
// This is the most efficient way to get geometry into the OpenGL
// graphics pipeline.
//
// Each vertex is specified as four attributes which are made
// available in a vertex shader in the following attribute slots.
//
// position,        vec4,   attribute #0
// normal,          vec3,   attribute #1
// texture coord,   vec3,   attribute #2
// tangent,         vec3,   attribute #3
//
// An instance of any of these shapes is create with a single call:
//    unsigned int obj = CreateSphere(divisions, &quadCount);
// and drawn by:
//    glBindVertexArray(vaoID);
//    glDrawElements(GL_TRIANGLES, vertexcount, GL_UNSIGNED_INT, 0);
//    glBindVertexArray(0);
////////////////////////////////////////////////////////////////////////

#ifndef _SHAPES
#define _SHAPES

#include "transform.h"
#include <vector>

class Shape
{
public:

    // The OpenGL identifier of this VAO
    unsigned int vaoID;

    // Data arrays
    std::vector<vec4> Pnt;
    std::vector<vec3> Nrm;
    std::vector<vec2> Tex;
    std::vector<vec3> Tan;

    // Lighting information
    vec3 diffuseColor, specularColor;
    float shininess;

    // Geometry defined by indices into data arrays
    std::vector<ivec3> Tri;
    unsigned int count;

    // Defined by SetTransform by scanning data arrays
    vec3 minP, maxP;
    vec3 center;
    float size;
    bool animate;

    // Constructor and destructor
    Shape() :animate(false) {}
    virtual ~Shape() {}

    virtual void MakeVAO();
    virtual void DrawVAO();
};

class SphereObj: public Shape
{
public:
    SphereObj(const int n);
};

class PlaneObj: public Shape
{
public:
    PlaneObj(const float range, const int n);
};

class ProceduralGround: public Shape
{
public:
    float range;
    float octaves;
    float persistence;
    float scale;
    float low;
    float high;
    float xoff;

    ProceduralGround(const float _range, const int n,
                     const float _octaves, const float _persistence, const float _scale,
                     const float _low, const float _high);
    float HeightAt(const float x, const float y);
};

#endif
