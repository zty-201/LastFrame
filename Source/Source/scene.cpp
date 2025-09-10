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
 // interactions.  All of them can be used to draw the scene.

 #include "pch.hpp"
 #include "global.hpp"
 #include "scene.h"
 #include "Mesh.h"  


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

 mat4 Identity = mat4(1.0f);  // FIX: Initialize to identity matrix!

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
void Scene::InitializeScene()
{
    std::cout << "=== Scene::InitializeScene() starting ===" << std::endl;
    
    std::cout << "=== Scene::InitializeScene() starting ===" << std::endl;
    
    glEnable(GL_DEPTH_TEST);
    std::cout << "Depth test enabled" << std::endl;
    std::cout << "Depth test enabled" << std::endl;
    std::cout << "Depth test enabled" << std::endl;
    std::cout << "Depth test enabled" << std::endl;

    // Set initial light parameters
    lightSpin = 150.0f;
    lightTilt = 5.0f;
    lightDist = 100.0f;
    std::cout << "Light parameters set: spin=" << lightSpin << ", tilt=" << lightTilt << ", dist=" << lightDist << std::endl;
    
    w_down = false;
    s_down = false;
    a_down = false;
    d_down = false;
    nav = true;
    spin = 0.0f;        // Looking along negative Y axis
    tilt = 0.0f;       // Look horizontally
    eye = vec3(0.0f, 0.0f, 50.0f); // Move camera much further back
    speed = 300.0f/30.0f;
    last_time = static_cast<float>(glfwGetTime());
    tr = vec3(0.0f, 0.0f, 25.0f);
   
    ry = 0.8f;          // Wider field of view
    front = 0.1f;       // See closer objects
    back = 5000.0f;     // See far away objects

    std::cout << "Camera parameters set: eye=(" << eye.x << "," << eye.y << "," << eye.z << ")" << std::endl;

    objectRoot = new Object(nullptr, nullId);
    std::cout << "Object root created" << std::endl;
    std::cout << "Object root created" << std::endl;
        
    // Create the lighting shader program from source code files.
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("./Source/Source/lightingPhong.vert", GL_VERTEX_SHADER);
    lightingProgram->AddShader("./Source/Source/lightingPhong.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->GetProgramID(), 0, "vertex");
    glBindAttribLocation(lightingProgram->GetProgramID(), 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->GetProgramID(), 2, "vertexTexture");
    glBindAttribLocation(lightingProgram->GetProgramID(), 3, "vertexTangent");
    lightingProgram->LinkProgram();
    
    // Create a reasonably sized sphere that you can see from outside
    std::shared_ptr<Mesh> sphereMesh = Mesh::CreateSphere(30);
    Object* testSphere = new Object(sphereMesh, debugId, 
                                   vec3(1.0f, 0.3f, 0.3f), // Bright red
                                   vec3(1.0f), 120);
    
    // Make it smaller - scale 2x instead of 20x
    mat4 sphereTransform = Translate(0.0f, 0.0f, 0.0f) * Scale(2.0f, 2.0f, 2.0f);
    objectRoot->instances.push_back({testSphere, sphereTransform});
    
    // Print camera position for debugging
    std::cout << "Camera initial position: " << eye.x << ", " << eye.y << ", " << eye.z << std::endl;
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

    // Simple movement 
    eye += stepDist*stepDir;

    if (nav)
        WorldView = lookAt(eye, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
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
    glClearColor(0.5, 0.5, 0.5, 1.0);
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

    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(viewInverse));
    
    // ADD THIS: Set NormalTr matrix
    mat4 normalTr = transpose(inverse(Identity)); // For identity transform, this is just Identity
    loc = glGetUniformLocation(programId, "NormalTr");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(normalTr));
    
    loc = glGetUniformLocation(programId, "lightPos");
    glUniform3fv(loc, 1, &(lightPos[0]));   
    loc = glGetUniformLocation(programId, "mode");
    glUniform1i(loc, mode);

    glPolygonOffset(1.0, 1.0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    objectRoot->Draw(lightingProgram, Identity);
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    // TEMPORARILY COMMENT OUT WIREFRAME DRAWING
    /*
    // Debug drawing - add wireframe overlay to see object shapes
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    objectRoot->Draw(lightingProgram, Identity);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    */
    
    // Turn off the shader
    lightingProgram->UnuseShader();

    // Enable face culling to see only front faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

                                                    
void Scene::DestroyScene()
{
    // No more student code to destroy
}
