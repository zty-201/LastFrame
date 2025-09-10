////////////////////////////////////////////////////////////////////////
 // The scene class contains all the parameters needed to define and
 // draw a simple scene, including:
 //   * Geometry
 //   * Light parameters
 //   * Material properties
 //   * viewport size parameters
 //   * Viewing transformation values
 //   * others ...
 //
 // Some of these parameters are set when the scene is built, and
 // others are set by the framework in response to user mouse/keyboard
 // intewractions.  All of them can be used to draw the scene.

 #include "pch.hpp"
 #include "global.hpp"
 #include "scene.h"


 #if 0
 std::random_device randomSeed;
 std::mt19937_64 RG(randomSeed());
 #else
 std::string seed = "A constant seed.";
 std::seed_seq constSeed (seed.begin(),seed.end());
 std::mt19937_64 RG(constSeed);
 #endif

 std::uniform_real_distribution<> myrandom(0.0, 1.0);
 std::uniform_real_distribution<> BALrandom(-1.0, 1.0);
 // Call myrandom(RG) to get a uniformly distributed random number in [0,1].

 const float rad = PI/180.0f;    // Convert degrees to radians

 mat4 Identity;

 const float grndSize = 60.0f;    // Island radius;  Minimum about 20;  Maximum 1000 or so
 const float grndOctaves = 4.0f;  // Number of levels of detail to compute
 const float grndFreq = 0.03f;    // Number of hills per (approx) 50m
 const float grndPersistence = 0.03f; // Terrain roughness: Slight:0.01  rough:0.05
 const float grndLow = -3.0f;         // Lowest extent below sea level
 const float grndHigh = 5.0f;        // Highest extent above sea level


// Create an RGB color from human friendly parameters: hue, saturation, value
vec3 HSV2RGB(const float h, const float s, const float v)
{
    if (s == 0.0)
        return vec3(v,v,v);

    int i = (int)(h*6.0) % 6;
    float f = (h*6.0f) - i;
    float p = v*(1.0f - s);
    float q = v*(1.0f - s*f);
    float t = v*(1.0f - s*(1.0f-f));
    if      (i == 0)  return vec3(v,t,p);
    else if (i == 1)  return vec3(q,v,p);
    else if (i == 2)  return vec3(p,v,t);
    else if (i == 3)  return vec3(p,q,v);
    else if (i == 4)  return vec3(t,p,v);
    else   /*i == 5*/ return vec3(v,p,q);
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, shape VAOs, and shader programs as well as setting a
// number of other parameters.
int total(int p, int e, int g) { return (4*p*p)*(2*e+1)*(2*e+1) + 2*g*g; }
void Scene::InitializeScene()
{
    while (total(polyCount,ellipsoidCount, grndCount) <= GOAL) {
        polyCount++;
        
        if (total(polyCount,ellipsoidCount+1, grndCount) <= GOAL) {
            ellipsoidCount += 1; }
        if (total(polyCount,ellipsoidCount, grndCount+10) <= GOAL) {
            grndCount += 10; }
    }
    
    glEnable(GL_DEPTH_TEST);

    // Set initial light parameters
    lightSpin = 150.0f;
    lightTilt = 5.0f;
    lightDist = 100.0f;
    
    w_down = false;
    s_down = false;
    a_down = false;
    d_down = false;
    nav = true;
    spin = 0.0f;        // Looking along negative Y axis
    tilt = 45.0f;       // Looking downward at 45 degrees
    eye = vec3(0.0f, 20.0f, 20.0f); // Position above and behind origin
    speed = 300.0f/30.0f;
    last_time = static_cast<float>(glfwGetTime());
    tr = vec3(0.0f, 0.0f, 25.0f);
   
    ry = 0.8f;          // Wider field of view
    front = 0.1f;       // See closer objects
    back = 5000.0f;     // See far away objects

    objectRoot = new Object(NULL, nullId);

    // Create the lighting shader program from source code files.
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("./Source/Source/lightingPhong.vert", GL_VERTEX_SHADER);
    lightingProgram->AddShader("./Source/Source/lightingPhong.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->GetProgramID(), 0, "vertex");
    glBindAttribLocation(lightingProgram->GetProgramID(), 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->GetProgramID(), 2, "vertexTexture");
    glBindAttribLocation(lightingProgram->GetProgramID(), 3, "vertexTangent");
    lightingProgram->LinkProgram();
    
    // Check for OpenGL errors
    GLenum err;
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL error after shader compilation: " << err << std::endl;
    }

    // Test if shader is valid
    GLint status;
    glGetProgramiv(lightingProgram->GetProgramID(), GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        std::cout << "Shader linking failed!" << std::endl;
        // Get log
        GLint logLen;
        glGetProgramiv(lightingProgram->GetProgramID(), GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            char* log = new char[logLen];
            glGetProgramInfoLog(lightingProgram->GetProgramID(), logLen, NULL, log);
            std::cout << "Linking log: " << log << std::endl;
            delete[] log;
        }
    }
    
    // Create all the Polygon shapes
    proceduralground = new ProceduralGround(grndSize, grndCount,
                                     grndOctaves, grndFreq, grndPersistence,
                                     grndLow, grndHigh);
    
    Shape* SpherePolygons = new SphereObj(polyCount);
    Shape* SeaPolygons = new PlaneObj(2000.0, 1);
    Shape* GroundPolygons = proceduralground;

    // Various colors used in the subsequent models
    vec3 woodColor(87.0/255.0, 51.0/255.0, 35.0/255.0);
    vec3 brickColor(134.0/255.0, 60.0/255.0, 56.0/255.0);
    vec3 floorColor(6*16/255.0, 5.5*16/255.0, 3*16/255.0);
    vec3 brassColor(0.5, 0.5, 0.1);
    vec3 grassColor(62.0/255.0, 102.0/255.0, 38.0/255.0);
    vec3 waterColor(0.3, 0.3, 1.0);

    vec3 black(0.0, 0.0, 0.0);
    vec3 brightSpec(0.5, 0.5, 0.5);
    vec3 polishedSpec(0.3, 0.3, 0.3);
 
    // Creates all the models from which the scene is composed.

    ground     = new Object(GroundPolygons, groundId, grassColor, black, 1);
    sea        = new Object(SeaPolygons, seaId, waterColor, brightSpec, 120);

    for (int i=-ellipsoidCount; i<=ellipsoidCount; i++) {
        for (int j=-ellipsoidCount; j<=ellipsoidCount; j++) {
            vec3 hue = HSV2RGB(static_cast<float>(myrandom(RG)), 1.0f, 1.0f);
            float r = 0.5f*(0.5f+2* static_cast<float>(myrandom(RG)));
            float h = 2* static_cast<float>(myrandom(RG));
            vec3 s(r, r, h);
            int v = (i==0 && j==0) ? 0 : 1;
            vec3 t(5*i + v*2* static_cast<float>(BALrandom(RG)),
                        5*j + v*2* static_cast<float>(BALrandom(RG)),
                        0.0);
            t[2] = proceduralground->HeightAt(t[0], t[1]) + s[2]*0.9f;
            Object* ellipsoid = new Object(SpherePolygons, 4, hue, vec3(0.5f), 120);
            objectRoot->add(ellipsoid, Translate(t)*Scale(s)); 
        } 
    }

    // Scene is composed of some scene objects, and the sky, ground, and sea
    objectRoot->add(ground);
    objectRoot->add(sea); 

    // Count the number of triangles
    triangleCount = 0;
    for (INSTANCE instance : objectRoot->instances) {
        Object* object = instance.first;
        std::vector<ivec3>& Tri = object->shape->Tri; 
        triangleCount += static_cast<int>(Tri.size());
    }

    fsq = new PlaneObj(1.0, 1);

    // Make ground more visible
    ground->diffuseColor = vec3(0.5f, 0.8f, 0.3f); // Brighter green
    sea->diffuseColor = vec3(0.1f, 0.4f, 0.9f);     // Brighter blue

    // Add a large visible test object in the scene center
    Object* testSphere = new Object(new SphereObj(30), 4, 
                                   vec3(1.0f, 0.3f, 0.3f), // Bright red
                                   vec3(1.0f), 120);
    objectRoot->add(testSphere, Translate(0.0f, 0.0f, 10.0f) * Scale(5.0f, 5.0f, 5.0f));

    // Print camera position
    std::cout << "Camera initial position: " << eye.x << ", " << eye.y << ", " << eye.z << std::endl;
    std::cout << "Ground count: " << grndCount << " Ellipsoid count: " << ellipsoidCount << std::endl;
    std::cout << "Triangle count: " << triangleCount << std::endl;

    // Make ground more visible
    ground->diffuseColor = vec3(0.5f, 0.8f, 0.3f); // Brighter green
    sea->diffuseColor = vec3(0.1f, 0.4f, 0.9f);     // Brighter blue
    }

void Scene::DrawGUI()
{
    // ImGui integration removed for simplicity
}

void Scene::BuildTransforms()
{
    // Work out the eye position as the user move it with the WASD keys.
    float now = static_cast<float>(glfwGetTime());
    float stepDist = (now-last_time)*speed;
    last_time = now;
    vec3 stepDir;
    if (w_down)
        stepDir = vec3(sin(spin*rad), cos(spin*rad), 0.0);
    if (s_down)
        stepDir = -vec3(sin(spin*rad), cos(spin*rad), 0.0);
    if (d_down)
        stepDir = vec3(cos(spin*rad), -sin(spin*rad), 0.0);
    if (a_down)
        stepDir = -vec3(cos(spin*rad), -sin(spin*rad), 0.0);

    // Simple movement without student code check
    eye += stepDist*stepDir;
    if (proceduralground)
        eye[2] = proceduralground->HeightAt(eye[0], eye[1]) + 1.5f; // Set the height of the eye

    if (nav)
        WorldView = Rotate(0, tilt-90)*Rotate(2, spin) *Translate(-eye[0], -eye[1], -eye[2]);
    else
        WorldView = Translate(tr[0], tr[1], -tr[2]) *Rotate(0, tilt-90)*Rotate(2, spin);
    WorldProj = Perspective((ry*width)/height, ry, front, (mode==0) ? 1000 : back);
}

void Scene::TraceScene()
{
    // Simplified to not use raytracing functionality
    glfwGetFramebufferSize(window, &width, &height);
    
    // Calculate the light's position from lightSpin, lightTilt, lightDist
    lightPos = vec3(lightDist*cos(lightSpin*rad)*sin(lightTilt*rad),
                     lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
                     lightDist*cos(lightTilt*rad));
    
    BuildTransforms();

    // The lighting algorithm needs the inverse of the WorldView matrix
    viewInverse = inverse(WorldView);
    projInverse = inverse(WorldProj);
}

////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be
// drawn. (Which is often: 60 times per second or more is common.)
void Scene::DrawScene()
{
    // Set the viewport
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Ensure proper OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Try with culling disabled first
    glDisable(GL_CULL_FACE);
    
    // Clear with a distinctive color
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // Blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate the light's position from lightSpin, lightTilt, lightDist
    lightPos = vec3(lightDist*cos(lightSpin*rad)*sin(lightTilt*rad),
                     lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
                     lightDist*cos(lightTilt*rad));

    BuildTransforms();

    // The lighting algorithm needs the inverse of the WorldView matrix
    viewInverse = inverse(WorldView);
    projInverse = inverse(WorldProj);
    
    int loc, programId;

    // Lighting pass
    lightingProgram->UseShader();
    programId = lightingProgram->GetProgramID();

    // Set the viewport, and clear the screen
    glViewport(0, 0, width, height);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(viewInverse));
    loc = glGetUniformLocation(programId, "lightPos");
    glUniform3fv(loc, 1, &(lightPos[0]));   
    loc = glGetUniformLocation(programId, "mode");
    glUniform1i(loc, mode);

    glPolygonOffset(1.0, 1.0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    objectRoot->Draw(lightingProgram, Identity);
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    // Debug drawing - add wireframe overlay to see object shapes
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    objectRoot->Draw(lightingProgram, Identity);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Turn off the shader
    lightingProgram->UnuseShader();

}

                                                    
void Scene::DestroyScene()
{
    // No more student code to destroy
}
