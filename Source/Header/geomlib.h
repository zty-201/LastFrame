///////////////////////////////////////////////////////////////////////
// Geometric objects (Line, Box, Sphere, Plane, Triangle) and operations.
////////////////////////////////////////////////////////////////////////

#if !defined(_GEOMLIB_H)
#define _GEOMLIB_H

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>          // For printing GLM objects with to_string
//#include "object.h"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::ivec3;
using glm::length;
using glm::normalize;
using glm::dot;
using glm::cross;

// Forward declarations:
class Line;                     // Defined by: point, vector
class Segment;                  // Defined by: two points
class Ray;                      // Defined by: point, vector

class Box;                      // Defined by: two corner points
class Sphere;                   // Defined by: center and radius
class Plane;                    // Defined by: normal and d
class Triangle;                 // Defined by: three points
class Object;

struct Unimplemented {};       // Marks code to be implemented by students.

void geomlibUnitTests();

// Utility functions:
float Distance(const vec3& point, const Line& line); 
float Distance(const vec3& point, const Plane& plane);
bool Coplanar(const vec3& A,const vec3& B,
              const vec3& C, const vec3& D);

////////////////////////////////////////////////////////////////////////
// Line
////////////////////////////////////////////////////////////////////////
class Line
{
public:
    vec3 point;
    vec3 vector;
    
    // Constructors
    Line() : point(), vector() {return;}
    Line(const vec3& p, const vec3& v) : point(p),vector(v) {return;}
    vec3 lerp(const float t) const { return(point+t*vector); }
};

// Utility functions:
float AngleBetween(const Line& line1, const Line& line2); 
bool Coplanar(const Line& line1, const Line& line2); 
bool Parallel(const Line& line1, const Line& line2); 
bool Perpendicular(const Line& line1, const Line& line2);
 
float AngleBetween(const Line& line, const Plane& plane); 
bool Parallel(const Line& line, const Plane& plane); 
bool Perpendicular(const Line& line, const Plane& plane); 
bool Intersects(const Line& line, const Plane& plane, float *rt=NULL); 

////////////////////////////////////////////////////////////////////////
// Segment
////////////////////////////////////////////////////////////////////////
class Segment
{
public:
    vec3 point1;
    vec3 point2;
    
    // Constructors
    Segment() : point1(), point2() {return;}
    Segment(const vec3& p1, const vec3& p2)
        : point1(p1), point2(p2) {return;}
    vec3 lerp(const float t) const { return((1.0f-t)*point1+t*point2); }

    // Utility methods
    bool contains(const vec3& point) const;

};

// Utility functions:
bool Intersects(const Segment& seg, const Triangle& tri, float *rt=NULL);


////////////////////////////////////////////////////////////////////////
// Ray
////////////////////////////////////////////////////////////////////////
class Ray
{
public:
    vec3 origin;
    vec3 direction;
    
    // Constructor
    Ray() : origin(), direction() {return;}
    Ray(const vec3& o, const vec3& d)
        : origin(o), direction(d) {return;} 
    vec3 lerp(const float t) const { return(origin+t*direction); }

    // Utility method
    bool contains(const vec3& point, float *rt=NULL) const;
    // Returns paramter of intersection if containment is true and t != NULL
};

// Utility functions:
bool Intersects(const Ray& ray, const Sphere& sphere, float *rt=NULL); 
bool Intersects(const Ray& ray, const Triangle& tri,  float *rt=NULL,
                float *ru=NULL, float *rv=NULL);
bool Intersects(const Ray& ray, const Box& box,       float *rt=NULL);


////////////////////////////////////////////////////////////////////////
// Box
////////////////////////////////////////////////////////////////////////
class Box
{
public:
    vec3 center;    // Center point
    vec3 extents;   // Center to corner half extents.
    
    // Constructor
    Box() {return;}
    Box(const vec3& c, const vec3& e) : center(c), extents(e) {return;}

    // Utility method
    bool contains(const vec3& point) const;
};


////////////////////////////////////////////////////////////////////////
// Sphere
////////////////////////////////////////////////////////////////////////
class Sphere
{
public:
    vec3 center;
    float radius;
    
    // Constructors
    Sphere() : center(), radius(0) {return;}
    Sphere(const vec3& c, const float r) : center(c), radius(r) {return;}
};


////////////////////////////////////////////////////////////////////////
// Plane
////////////////////////////////////////////////////////////////////////
class Plane
{
public:
    float crds[4];
    
    // Constructor
    Plane(const float A=0, const float B=0, const float C=0, const float D=0)
        { crds[0]=A; crds[1]=B; crds[2]=C; crds[3]=D; }

    // Indexing operators.
    float& operator[](const unsigned int i) { return crds[i]; }
    const float& operator[](const unsigned int i) const { return crds[i]; } 

    // Utility methods.
    vec3 normal() const { return vec3(crds[0], crds[1], crds[2]); }
    float Evaluate(vec3 p) const { return crds[0]*p[0] + crds[1]*p[1] + crds[2]*p[2] + crds[3]; }
};

// Utility functions:
float AngleBetween(const Plane& plane1, const Plane& plane2);
bool Parallel(const Plane& plane1, const Plane& plane2);
bool Perpendicular(const Plane& plane1, const Plane& plane2);
bool Intersects(const Segment& seg, const Plane& plane, float *rt=NULL);


////////////////////////////////////////////////////////////////////////
// Triangle
////////////////////////////////////////////////////////////////////////
class Triangle
{
public:
    vec3 points[3];
    Object* objPtr = nullptr;
    // Constructor
    Triangle() {return;}
    Triangle(const vec3& p1, const vec3& p2, const vec3& p3)
        { points[0]=p1; points[1]=p2; points[2]=p3; }

        vec3& operator[](unsigned int i) { return points[i]; }
        const vec3& operator[](unsigned int i) const { return points[i]; } 

    // Utility method
    bool contains(const vec3& point) const;

    
};

// Utility function:
int Intersects(const Triangle& tri1, const Triangle& tri2,
           std::pair<vec3, vec3> *rpoints=0);


#endif // _GEOMLIB_H
