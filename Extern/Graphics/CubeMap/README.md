A cubemap is a series of 6 textures loaded as faces of a cubemap texture, typically used for skybox.
As such, files in this directory will not be automatically loaded.

To load a cubemap, place the texture files in this directory.
Next, edit the Config.txt file and use json to list out in this order: -x, +x, +y, -y, +z, -z.
{
  "cubemap_name": [
    "file1.png",
    "file2.png",
    "file3.png",
    "file4.png",
    "file5.png",
    "file6.png"
  ]
}
