////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.
#ifndef SCENE_H
#define SCENE_H
#include "geomlib.h"
#include "object.h"
#include "shader.h"
#include "transform.h"

enum ObjectIds {
    nullId	= 0,
    skyId	= 1,
    seaId	= 2,
    groundId	= 3,
    treeId	= 4,
    debugId	= 5
};

class Shader;

class Scene
{
public:
    int triangleCount;
    GLFWwindow* window;

    // Light parameters
    float lightSpin, lightTilt, lightDist;
    vec3 lightPos;

    bool drawReflective;
    bool nav;
    bool w_down, s_down, a_down, d_down;
    float spin, tilt, speed, ry, front, back;
    vec3 eye, tr;
    float last_time;
    int mode; // Extra mode indicator hooked up to number keys and sent to shader
    
    // Viewport
    int width, height;

    // Ray traced image
    int raysize = 0;
    float* rayimage = NULL;
    std::shared_ptr<Mesh> fsq; // Changed from PlaneObj*
    ShaderProgram* raytraceProgram;
    unsigned int rtTextureId = 0;

    // Transformations
    mat4 WorldProj, WorldView, viewInverse, projInverse;

    // All objects in the scene are children of this single root object.
    Object* objectRoot;
    Object *sky, *ground, *sea, *objects;

    std::shared_ptr<Mesh> proceduralground; // Changed from ProceduralGround*

    // Shader programs
    ShaderProgram* lightingProgram;

    void InitializeScene();
    void BuildTransforms();
    void DrawGUI();
    void DrawScene();
    void TraceScene();
    void DestroyScene();

    // ImGui variables
    bool show_demo_window;

    // Detail control variables 
    int polyCount = 3;       // Controls sphere polygon count
    int ellipsoidCount = 2;  // Controls number of ellipsoids
    int grndCount = 20;      // Controls ground detail
    int GOAL = 10000;        // Target complexity value
};

#endif
