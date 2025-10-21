A cubemap is a series of 6 textures loaded as faces of a cubemap texture, typically used for skybox.
As such, files in this directory will not be automatically loaded.

To load a cubemap, place the texture files in this directory.
Next, edit the Config.json file and use json to list out in this order: -x, +x, +y, -y, +z, -z.
{
  "night_skybox": [
    "Extern/Graphics/CubeMap/night_negx.png",
    "Extern/Graphics/CubeMap/night_posx.png",
    "Extern/Graphics/CubeMap/night_posy.png",
    "Extern/Graphics/CubeMap/night_negy.png",
    "Extern/Graphics/CubeMap/night_posz.png",
    "Extern/Graphics/CubeMap/night_negz.png"
  ]
}
