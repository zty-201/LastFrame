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
#include "pch.hpp"
#include "object.h"
#include "shader.h"

#include "shapes.h"
#include "transform.h"

               // For gluErrorString
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line object.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }


Object::Object(Shape* _shape, const int _objectId,
               const vec3 _diffuseColor, const vec3 _specularColor, const float _shininess)
    : diffuseColor(_diffuseColor), specularColor(_specularColor), shininess(_shininess),
      shape(_shape), objectId(_objectId)
     
{}


void Object::Draw(ShaderProgram* program, mat4& objectTr)
{
    
    // Inform the shader of the surface values Kd, Ks, and alpha.
    int loc = glGetUniformLocation(program->GetProgramID(), "diffuse");
    glUniform3fv(loc, 1, &diffuseColor[0]);

    loc = glGetUniformLocation(program->GetProgramID(), "specular");
    glUniform3fv(loc, 1, &specularColor[0]);

    loc = glGetUniformLocation(program->GetProgramID(), "shininess");
    glUniform1f(loc, shininess);

    // Inform the shader of which object is being drawn so it can make
    // object specific decisions.
    loc = glGetUniformLocation(program->GetProgramID(), "objectId");
    glUniform1i(loc, objectId);

    // Inform the shader of this object's model transformation.  The
    // inverse of the model transformation, needed for transforming
    // normals, is calculated and passed to the shader here.
    loc = glGetUniformLocation(program->GetProgramID(), "ModelTr");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(objectTr));
    
    mat4 inv = inverse(objectTr);
    loc = glGetUniformLocation(program->GetProgramID(), "NormalTr");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(inv));

    // If this object has an associated texture, this is the place to
    // load the texture into a texture-unit of your choice and inform
    // the shader program of the texture-unit number.  See
    // Texture::Bind for the 4 lines of code to do exactly that.
    

    // Draw this object
    if (shape)
        shape->DrawVAO();


    // Recursively draw each sub-objects, each with its own transformation.
    for (int i=0;  i<instances.size();  i++) {
        mat4 itr = objectTr*instances[i].second;
        instances[i].first->Draw(program, itr); }
    
}
