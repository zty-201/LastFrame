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

#include "pch.hpp"
#include "macros.hpp"
#include "shapes.h"

const float rad = PI/180.0f;

void pushquad(std::vector<ivec3> &Tri, int i, int j, int k, int l)
{
    Tri.push_back(ivec3(i,j,k));
    Tri.push_back(ivec3(i,k,l));
}

// Batch up all the data defining a shape to be drawn (example: the
// teapot) as a Vertex Array object (VAO) and send it to the graphics
// card.  Return an OpenGL identifier for the created VAO.
unsigned int VaoFromTris(std::vector<vec4> Pnt,
                         std::vector<vec3> Nrm,
                         std::vector<vec2> Tex,
                         std::vector<vec3> Tan,
                         std::vector<ivec3> Tri)
{
    //printf("VaoFromTris %ld %ld\n", Pnt.size(), Tri.size());
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    GLuint Pbuff;
    glGenBuffers(1, &Pbuff);
    glBindBuffer(GL_ARRAY_BUFFER, Pbuff);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*Pnt.size(),
                 &Pnt[0][0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (Nrm.size() > 0) {
        GLuint Nbuff;
        glGenBuffers(1, &Nbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Nbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*Nrm.size(),
                     &Nrm[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    if (Tex.size() > 0) {
        GLuint Tbuff;
        glGenBuffers(1, &Tbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Tbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*Tex.size(),
                     &Tex[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    if (Tan.size() > 0) {
        GLuint Dbuff;
        glGenBuffers(1, &Dbuff);
        glBindBuffer(GL_ARRAY_BUFFER, Dbuff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*Tan.size(),
                     &Tan[0][0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); }

    GLuint Ibuff;
    glGenBuffers(1, &Ibuff);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Ibuff);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*3*Tri.size(),
                 &Tri[0][0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    return vaoID;
}

void Shape::MakeVAO()
{
    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = static_cast<unsigned int>(Tri.size());
}

void Shape::DrawVAO()
{

    glBindVertexArray(vaoID);
    glDrawElements(GL_TRIANGLES, 3*count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}

////////////////////////////////////////////////////////////////////////
// Generates a sphere of radius 1.0 centered at the origin.
//   n specifies the number of polygonal subdivisions
SphereObj::SphereObj(const int n)
{
    diffuseColor = vec3(0.5, 0.5, 1.0);
    specularColor = vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    //float d = 2.0f*PI/float(n*2);
    for (int i=0;  i<=n*2;  i++) {
        float s = i*2.0f*PI/float(n*2);
        for (int j=0;  j<=n;  j++) {
            float t = j*PI/float(n);
            float x = cos(s)*sin(t);
            float y = sin(s)*sin(t);
            float z = cos(t);
            Pnt.push_back(vec4(x,y,z,1.0f));
            Nrm.push_back(vec3(x,y,z));
            Tex.push_back(vec2(s/(2*PI), t/PI));
            Tan.push_back(vec3(-sin(s), cos(s), 0.0));
            if (i>0 && j>0) {
                pushquad(Tri, (i-1)*(n+1) + (j-1),
                                      (i-1)*(n+1) + (j),
                                      (i  )*(n+1) + (j),
                                      (i  )*(n+1) + (j-1)); } } }
    MakeVAO();
}


////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
PlaneObj::PlaneObj(const float r, const int n)
{
    diffuseColor = vec3(0.3, 0.2, 0.1);
    specularColor = vec3(1.0, 1.0, 1.0);
    shininess = 120.0;

    for (int i=0;  i<=n;  i++) {
        float s = i/float(n);
        for (int j=0;  j<=n;  j++) {
            float t = j/float(n);
            Pnt.push_back(vec4(s*2.0*r-r, t*2.0*r-r, 0.0, 1.0));
            Nrm.push_back(vec3(0.0, 0.0, 1.0));
            Tex.push_back(vec2(s, t));
            Tan.push_back(vec3(1.0, 0.0, 0.0));
            if (i>0 && j>0) {
                pushquad(Tri, (i-1)*(n+1) + (j-1),
                                      (i-1)*(n+1) + (j),
                                      (i  )*(n+1) + (j),
                                      (i  )*(n+1) + (j-1)); } } }

    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = static_cast<int>(Tri.size());
}

////////////////////////////////////////////////////////////////////////
// Generates a plane with normals, texture coords, and tangent vectors
// from an n by n grid of small quads.  A single quad might have been
// sufficient, but that works poorly with the reflection map.
ProceduralGround::ProceduralGround(const float _range, const int n,
                     const float _octaves, const float _persistence, const float _scale,
                     const float _low, const float _high)
    :range(_range), octaves(_octaves), persistence(_persistence), scale(_scale), 
     low(_low), high(_high)
{
    diffuseColor = vec3(0.3, 0.2, 0.1);
    specularColor = vec3(1.0, 1.0, 1.0);
    shininess = 10.0;
    specularColor = vec3(0.0, 0.0, 0.0);
    xoff = 0.0; //range*( time(NULL)%1000 );

    float h = 0.001f;
    for (int i=0;  i<=n;  i++) {
        float s = i/float(n);
        for (int j=0;  j<=n;  j++) {
            float t = j/float(n);
            float x = s*2.0f*range-range;
            float y = t*2.0f*range-range;
            float z = HeightAt(x, y);
            float zu = HeightAt(x+h, y);
            float zv = HeightAt(x, y+h);
            Pnt.push_back(vec4(x, y, z, 1.0));
            vec3 du(1.0, 0.0, (zu-z)/h);
            vec3 dv(0.0, 1.0, (zv-z)/h);
            Nrm.push_back(normalize(cross(du,dv)));
            Tex.push_back(vec2(s, t));
            Tan.push_back(vec3(1.0, 0.0, 0.0));
            if (i>0 && j>0) {
                pushquad(Tri,
                         (i-1)*(n+1) + (j-1),
                         (i-1)*(n+1) + (j),
                         (i  )*(n+1) + (j),
                         (i  )*(n+1) + (j-1)); } } }

    vaoID = VaoFromTris(Pnt, Nrm, Tex, Tan, Tri);
    count = static_cast<int>(Tri.size());
}

float ProceduralGround::HeightAt(const float x, const float y)
{
    // Add some simple hills to make terrain more visible
    float noise = sin(x*0.1f) * cos(y*0.1f) * 3.0f;
    // Use existing smoothing code...
    float rs = smoothstep(range-20.0f, range, sqrtf(x*x+y*y));
    float z = (1-rs)*noise + rs*low;
    
    float hs = smoothstep(15.0f, 45.0f,
                         length(vec3(x,y,0)-vec3(0,0,0)));
    return (1-hs)*5.0f + hs*z; // Make center higher (5.0f)
}
