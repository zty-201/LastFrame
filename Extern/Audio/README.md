This folder is used to store audio files.
All audio files in this folder are loaded automatically at initialize time,
where they are identified using their file_name (hashed into an xresource::instance_guid integer).
Alternatively, add them to the config file.

{
    "lizard" : {
        "filepath" : "Extern/Audio/lizard.mp3",
        "isLoop" : false
    }
}

Note:
- Audio assets are kept separate from their configuration settings (like volume). This allows a single loaded asset to be played in different ways based on the different scenes and context in the game.
- Store configuration settings in the same component that holds a handle to that specific audio asset. Each component may have different configuration settings as a result, and it allows these settings to be change on the fly instead of being fixed at initialization.
- The only exception is (loop), because it needs to be set at load-time.

