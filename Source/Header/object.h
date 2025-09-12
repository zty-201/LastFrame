////////////////////////////////////////////////////////////////////////
// A lightweight class representing an instance of an object that can
// be drawn onscreen.  An Object consists of a shape (batch of
// triangles), and various transformation, color and texture
// parameters.  It also contains a list of child objects to be drawn
// in a hierarchical fashion under the control of parent's
// transformations.
//
// Methods consist of a constructor, and a Draw procedure, and an
// append for building hierarchies of objects.
#ifndef _OBJECT
#define _OBJECT

#include "transform.h"
#include "Mesh.h" // Use Mesh.h instead of shapes.h
#include <vector>
#include <memory>

class ShaderProgram;

class Object
{
public:
    std::vector<std::pair<Object*,mat4> > instances;
    vec3 diffuseColor, specularColor;
    float shininess;
    int objectId;

    std::shared_ptr<Mesh> mesh; // Changed from Shape* to shared_ptr<Mesh>

    Object(std::shared_ptr<Mesh> _mesh, const int _objectId,
           const vec3 _diffuseColor=vec3(0.5,0.5,0.5),
           const vec3 _specularColor=vec3(0.5,0.5,0.5),
           const float _shininess=1.0f);

    void Draw(ShaderProgram* program, mat4& objectTr);
};

#endif
