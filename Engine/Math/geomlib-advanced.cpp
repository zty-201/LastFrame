///////////////////////////////////////////////////////////////////////
// Geometric objects (vec3s, vec3s, Planes, ...) and operations.
////////////////////////////////////////////////////////////////////////
#include "pch.hpp"
#include "geomlib.h"
#include <vector>
#include <cassert>
#include <float.h> // FLT_EPSILON

////////////////////////////////////////////////////////////////////////
// Distance methods
////////////////////////////////////////////////////////////////////////

// Return the distance from a point to a line.
float Distance(const vec3& point, const Line& line)
{
    const vec3 w = point - line.point;
    const vec3 v = line.vector;
    return length(cross(w, v)) / length(v);
}

// Return the distance from a point to a plane.
float Distance(const vec3& point, const Plane& plane)
{
    const float a = fabs(plane.Evaluate(point)); // This is the distance from the point to the plane.
    const float b = length(vec3(plane.crds[0], plane.crds[1], plane.crds[2]));

    return a / b;
}

////////////////////////////////////////////////////////////////////////
// Containment methods
////////////////////////////////////////////////////////////////////////

// Determines if point (known to be on a line) is contained within a segment.
bool Segment::contains(const vec3& point) const
{
    const vec3 AB = point2 - point1;
    const vec3 AP = point - point1;
    const float t = dot(AP, AB) / dot(AB, AB);
    if (t <= 1 && t >= 0) { // parallel
        return true;
    }
    return false;
}

// Determines if point (known to be on a line) is contained within a ray.
bool Ray::contains(const vec3& point, float *rt) const
{
    const vec3 MP = point - origin;
    const vec3 V = direction;
    const float t = dot(MP, V) / dot(V, V);
    if (t >= FLT_EPSILON && rt != nullptr) { // parallel
        *rt = t;
        return true;
    }
    return false;
}

// Determines if point is contained within a box.
bool Box::contains(const vec3& point) const
{
    if (fabs(center.x - point.x) <= extents.x &&
        fabs(center.y - point.y) <= extents.y &&
        fabs(center.z - point.z) <= extents.z) {
        return true;
    }
    return false;
}

// Determines if point (known to be on a plane) is contained within a triangle.
bool Triangle::contains(const vec3& point) const
{
    // triangle -> point a, b and c
    const vec3 AB = points[1] - points[0];
    const vec3 AC = points[2] - points[0];
    const vec3 normal = normalize(cross(AB, AC));
    const vec3 AP = point - points[0];
    const vec3 BP = point - points[1];
    const vec3 CP = point - points[2];
    const vec3 edgeAB = cross(normal, (points[1] - points[0]));
    const vec3 edgeBC = cross(normal, (points[2] - points[1]));
    const vec3 edgeCA = cross(normal, (points[0] - points[2]));


    if (dot(edgeAB, AP) >= -FLT_EPSILON &&
        dot(edgeBC, BP) >= -FLT_EPSILON &&
        dot(edgeCA, CP) >= -FLT_EPSILON) {
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////
// Intersects functions
// In the following Intersects function these rules apply:
//
// * Most test are to determine if a *unique* solution exists. (Or in
//   some cases up to two intersection points exist.)  Parallel
//   objects have either zero or infinitely many solutions and so
//   return false.
//
// * If a unique solution exists, a function value of true is
//   returned.  (Or in the cases where several solutions can exist,
//   the number of intersection parameters are returned.)
//
// * If a unique solution does exist, the calling program may provide
//   a memory location into which the intersection parameter can be
//   returned.  Such pointer may be NULL to indicate that the
//   intersection parameter is not to be returned.
//
////////////////////////////////////////////////////////////////////////

// Determines if  line and plane have a unique intersection.  
// If true and t is not NULL, returns intersection parameter.
bool Intersects(const Line& line, const Plane& plane, float *rt)
{
    const vec3 N = vec3(plane.crds[0], plane.crds[1], plane.crds[2]);
    const float denom = dot(N, line.vector);

    if (fabs(denom) < FLT_EPSILON || rt == nullptr) {
        return false; // Line is parallel to plane
    }

    float t = -plane.Evaluate(line.point) /
        denom;
    *rt = t;
    return true;
}

// Determines if  segment and plane have a unique intersection.  
// If true and rt is not NULL, returns intersection parameter.
bool Intersects(const Segment& seg, const Plane& plane, float *rt)
{
    const vec3 V = seg.point2 - seg.point1;
    const vec3 N = vec3(plane.crds[0], plane.crds[1], plane.crds[2]);
    const float denom = dot(N, V);

    if (fabs(denom) < FLT_EPSILON || rt == nullptr) {
        return false;
    }

    float t = -plane.Evaluate(seg.point1) /
        denom;
    *rt = t;
    return true;
}

// Determines if  ray and triangle have a unique intersection.  
// If true and rt is not NULL, returns intersection parameter.
bool AIntersects(const Ray& ray, const Triangle& tri, float *rt, float *ru, float *rv)
{
    const vec3 E1 = tri.points[1] - tri.points[0];
    const vec3 E2 = tri.points[2] - tri.points[0];
    const vec3 p = cross(ray.direction, E2);
    const float d = dot(p, E1);
    if (d == 0) {
        return false;
    }
    const vec3 S = ray.origin - tri.points[0];
    const float u = dot(p, S) / d;
    if (u < 0 || u > 1) {
        return false;
    }
    const vec3 q = cross(S, E1);
    const float v = dot(ray.direction, q) / d;
    if (v < 0 || u + v > 1) {
        return false;
    }
    const float t = dot(E2, q) / d;
    if (t < 0) {
        return false;
    }
    if (rt != nullptr) {
        *rt = t;
    }
    if (ru != nullptr) {
        *ru = u;
    }
    if (rv != nullptr) {
        *rv = v;
    }

    return true;
}
bool Intersects(const Ray& ray, const Triangle& tri, float *rt, float *ru, float *rv)
{
    float t, u, v;
    
    bool r = AIntersects(ray, tri, &t, &u, &v);

    // printf("TestRayTriangle(%d, %f, %f, %f, vec3(%f, %f, %f), vec3(%f, %f, %f), vec3(%f, %f, %f), vec3(%f, %f, %f), vec3(%f, %f, %f));\n",
    //        r, t, u, v,
    //        ray.origin.x, ray.origin.y, ray.origin.z,
    //        ray.direction.x, ray.direction.y, ray.direction.z,
    //        tri.points[0].x, tri.points[0].y, tri.points[0].z,
    //        tri.points[1].x, tri.points[1].y, tri.points[1].z,
    //        tri.points[2].x, tri.points[2].y, tri.points[2].z);
    
    if (rt) *rt = t;
    if (ru) *ru = u;
    if (rv) *rv = v;
    
    return r;
}

// Determines if  segment and triangle have a unique intersection.  
// If true and rt is not NULL, returns intersection parameter.
bool Intersects(const Segment& seg, const Triangle& tri, float *rt)
{
    const vec3 dir = seg.point2 - seg.point1;
    const Ray ray = { seg.point1, dir };
    return Intersects(ray, tri, rt, nullptr, nullptr);
}

// Determines if  ray and sphere intersect.  
// If so and rt is not NULL, returns intersection parameter.
bool Intersects(const Ray& ray, const Sphere& sphere, float *rt)
{
    if (rt == nullptr) {
        return false;
    }
    const vec3 V = ray.direction;
    const float r = sphere.radius;

    const vec3 MC = ray.origin - sphere.center;

    const float a = dot(V, V);
    const float b = 2.0f * dot(MC, V);
    const float c = dot(MC, MC) - r * r;

    const float discriminant = b * b - 4.0f * a * c;

    if (discriminant < FLT_EPSILON) {
        return false;
    }

    const float sqrt_disc = sqrt(discriminant);

    const float r1 = (-b - sqrt_disc) / (2 * a);
    const float r2 = (-b + sqrt_disc) / (2 * a);

    if (r1 >= FLT_EPSILON && r2 >= FLT_EPSILON) {
        if (r1 > r2) {
            *rt = r2;
        }
        else {
            *rt = r1;
        }
        return true;
    }
    return false;

}

// Determines if  ray and AABB intersect.  
// If so and rt is not NULL, returns intersection parameter.
bool Intersects(const Ray& ray, const Box& box, float *rt)
{
    if (rt == nullptr) {
        return false;
    }

    float t0 = -FLT_MAX;
    float t1 = FLT_MAX;

    const vec3& M = ray.origin;
    const vec3& V = ray.direction;
    const vec3 P0 = box.center - box.extents; 
    const vec3 P1 = box.center + box.extents;

    for (int i = 0; i < 3; ++i) {
        const float Mc = M[i];
        const float Vc = V[i];
        const float P0c = P0[i];
        const float P1c = P1[i];

        const float EPS = 1e-8f;           // any small value ¡Ö machine epsilon
        if (fabs(Vc) < EPS) {              // ray nearly parallel to slab
            if (Mc < P0c || Mc > P1c)      // origin outside the slab  miss
                return false;              // inside  keep searching
            continue;                      // go to next axis
        }
        else {
            float s0 = (P0c - Mc) / Vc;
            float s1 = (P1c - Mc) / Vc;
            if (s0 > s1) {
                std::swap(s0, s1);
            }

            t0 = fmax(t0, s0);
            t1 = fmin(t1, s1);

            if (t0 > t1) {
                return false;
            }
        }
    }

    if (t1 < FLT_EPSILON)
        return false;

    *rt = (t0 >= 0.0f) ? t0 : t1;


    return true;
}

// Determines if  triangles intersect.  
// If parallel, returns false. (This may be considered misleading.)
// If true and rpoint is not NULL, returns two edge/triangle intersections.
int Intersects(const Triangle& tri1, const Triangle& tri2,
			   std::pair<vec3, vec3> *rpoints)
{
    vec3 edges1[3][2] = {
        {tri1.points[0], tri1.points[1]},
        {tri1.points[1], tri1.points[2]},
        {tri1.points[2], tri1.points[0]}
    };

    vec3 edges2[3][2] = {
        {tri2.points[0], tri2.points[1]},
        {tri2.points[1], tri2.points[2]},
        {tri2.points[2], tri2.points[0]}
    };

    int count = 0;
    vec3 hits[2];

    
    for (int i = 0; i < 3; ++i) { // edges of tri1 against tri2
        Segment seg{ edges1[i][0], edges1[i][1] };
        float t;
        if (Intersects(seg, tri2, &t)) {
            vec3 hit = seg.point1 + t * (seg.point2 - seg.point1);
            if (count < 2) {
                hits[count] = hit;
            }
            ++count;
        }
    }

    
    for (int i = 0; i < 3; ++i) { // edges of tri2 against tri1
        Segment seg{ edges2[i][0], edges2[i][1] };
        float t;
        if (Intersects(seg, tri1, &t)) {
            vec3 hit = seg.point1 + t * (seg.point2 - seg.point1);
            if (count < 2) {
                hits[count] = hit;
            }
            ++count;
        }
    }


    if (rpoints && count >= 2) {
        *rpoints = std::make_pair(hits[0], hits[1]);
    }

    return count;

}

////////////////////////////////////////////////////////////////////////
// Geometric relationships
////////////////////////////////////////////////////////////////////////

// Compute angle between two geometric entities (in radians;  use acos)
float AngleBetween(const Plane& plane1, const Plane& plane2)
{
    const vec3 N1 = plane1.normal();
    const vec3 N2 = plane2.normal();
    return acos(dot(N1, N2) / (length(N1) * length(N2)));
}

// Compute angle between two geometric entities (in radians;  use acos)
float AngleBetween(const Line& line1, const Line& line2)
{
    const vec3 N1 = line1.vector;
    const vec3 N2 = line2.vector;
    return acos(dot(N1, N2) / (length(N1) * length(N2)));
}

// Compute angle between two geometric entities (in radians;  use acos)
float AngleBetween(const Line& line, const Plane& plane)
{
    const vec3 N = plane.normal();
    const vec3 V = line.vector;
    return glm::pi<float>() / 2 - acos(dot(N, V) / (length(N) * length(V)));
}

// Determine if two lines are coplanar
bool Coplanar(const Line& line1, const Line& line2)
{
    const vec3 N = cross(line1.vector, line2.vector);
    if (fabs(dot((line2.point - line1.point), N)) < FLT_EPSILON) {
        return true;
    }
    return false;
}

bool Perpendicular(const vec3& v1, const vec3& v2)
{
    if (fabs(dot(v1, v2)) < FLT_EPSILON) {
        return true;
    }
    return false;
}

// Determine if two vectors are parallel.
bool Parallel(const vec3& v1, const vec3& v2)
{
    if (fabs(dot(cross(v1, v2), cross(v1, v2)) < FLT_EPSILON)) {
        return true;
    }
    return false;
}

// Determine if two geometric entities are parallel.
bool Parallel(const Line& line1, const Line& line2)
{
    return Parallel(line1.vector, line2.vector);
}

// Determine if two geometric entities are parallel.
bool Parallel(const Line& line, const Plane& plane)
{
    return Perpendicular(plane.normal(), line.vector);
}

// Determine if two geometric entities are parallel.
bool Parallel(const Plane& plane1, const Plane& plane2)
{
    return Parallel(plane1.normal(), plane2.normal());
}

// Determine if two geometric entities are perpendicular.
bool Perpendicular(const Line& line1, const Line& line2)
{
    return Perpendicular(line1.vector, line2.vector);
}

// Determine if two geometric entities are perpendicular.
bool Perpendicular(const Line& line, const Plane& plane)
{
    return Parallel(plane.normal(), line.vector);
}

// Determine if two geometric entities are perpendicular.
bool Perpendicular(const Plane& plane1, const Plane& plane2)
{
    return Perpendicular(plane1.normal(), plane2.normal());
}
