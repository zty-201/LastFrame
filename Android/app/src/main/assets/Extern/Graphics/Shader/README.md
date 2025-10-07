A shader consists of 2 (or more) shader files (typically vertex and fragment).
Note that shader programs are mix and match, i.e. they can be reused multiple times.
As such, files in this directory will not be automatically loaded.


To load a shader, place the shader files in this directory.
Next, edit the Config.json file and use json to list out in this order:
{
  "shader_name" : {
    "vertex":   "Extern/Graphics/Shader/BasicVert.vert",
    "fragment": "Extern/Graphics/Shader/BasicFrag.frag"
  }
}
