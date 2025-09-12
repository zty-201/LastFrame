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
#include "Mesh.h"
#include "transform.h"

Object::Object(std::shared_ptr<Mesh> _mesh, const int _objectId,
               const vec3 _d, const vec3 _s, const float _f)
    : mesh(_mesh), objectId(_objectId), 
      diffuseColor(_d), specularColor(_s), shininess(_f)
{
    std::cout << "Object created with ID: " << objectId << std::endl;
    if (mesh) {
        std::cout << "  - Mesh is valid" << std::endl;
    } else {
        std::cout << "  - WARNING: Mesh is null!" << std::endl;
    }
}

void Object::Draw(ShaderProgram* program, mat4& objectTr) {
    static int drawCallCount = 0;
    drawCallCount++;
    
    // Debug the sphere (objectId 5) every time, others only every 60 calls
    bool shouldDebug = (objectId == 5) || (drawCallCount % 60 == 1);
    
    if (shouldDebug) {
        std::cout << "Object::Draw called (call #" << drawCallCount << ") for objectId: " << objectId << std::endl;
        std::cout << "  - mesh pointer: " << (mesh ? "VALID" : "NULL") << std::endl;
    }
    
    // Draw this object's mesh if it has one
    if (mesh) {
        if (shouldDebug) {
            std::cout << "  - Drawing mesh for object " << objectId << std::endl;
            // Debug the transformation matrix
            std::cout << "  - objectTr matrix: [" << objectTr[0][0] << "," << objectTr[0][1] << "," << objectTr[0][2] << "," << objectTr[0][3] << "]" << std::endl;
            std::cout << "                      [" << objectTr[1][0] << "," << objectTr[1][1] << "," << objectTr[1][2] << "," << objectTr[1][3] << "]" << std::endl;
            std::cout << "                      [" << objectTr[2][0] << "," << objectTr[2][1] << "," << objectTr[2][2] << "," << objectTr[2][3] << "]" << std::endl;
            std::cout << "                      [" << objectTr[3][0] << "," << objectTr[3][1] << "," << objectTr[3][2] << "," << objectTr[3][3] << "]" << std::endl;
        }
        
        int programId = program->GetProgramID();
        int loc;
        
        // Set the model transformation matrix
        mat4 modelTr = objectTr;
        loc = glGetUniformLocation(programId, "ModelTr");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(modelTr));
        if (shouldDebug) {
            if (loc == -1) {
                std::cout << "  - WARNING: ModelTr uniform not found" << std::endl;
            } else {
                std::cout << "  - ModelTr uniform set (location: " << loc << ")" << std::endl;
            }
        }
        
        // Set material properties
        loc = glGetUniformLocation(programId, "objectId");
        glUniform1i(loc, objectId);
        if (shouldDebug) {
            if (loc == -1) {
                std::cout << "  - WARNING: objectId uniform not found" << std::endl;
            } else {
                std::cout << "  - objectId uniform set to " << objectId << " (location: " << loc << ")" << std::endl;
            }
        }

        if (shouldDebug) {
            std::cout << "  - All uniforms set, calling mesh->Draw()" << std::endl;
        }
        
        // Draw the mesh
        mesh->Draw();
        
        if (shouldDebug) {
            std::cout << "  - mesh->Draw() completed" << std::endl;
        }
    } else if (shouldDebug) {
        std::cout << "  - No mesh to draw for this object" << std::endl;
    }
    
    // Draw child instances
    if (shouldDebug) {
        std::cout << "  - Drawing " << instances.size() << " child instances" << std::endl;
    }
    
    for (const auto& instance : instances) {
        mat4 childTr = objectTr * instance.second;
        if (shouldDebug) {
            std::cout << "  - Calling child Draw() for objectId: " << instance.first->objectId << std::endl;
        }
        instance.first->Draw(program, childTr);
    }
}
