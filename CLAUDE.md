# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**LastFrame** is a 3D horror game built with a custom C++20 engine ("EngineLibrary") by Team Cheerios (DigiPen). The engine targets Windows (Visual Studio) and Android, using an ECS (Entity-Component-System) architecture.

## Build System

**Prerequisites:** CMake 3.20+, Visual Studio 2022 (MSVC)

**Configure (no ASAN/code analysis — faster for development):**
```bat
Configure_VisualStudio_No_Analysis.bat
```
This runs `cmake --preset visual_studio_x64_no_analyze` and generates the solution in `build/VisualStudio/`.

**Configure (with ASAN + code analysis):**
```bat
Configure_VisualStudio.bat  # (if present) or cmake --preset visual_studio_x64
```

**Build & Run:** Open `build/VisualStudio/LastFrame.sln` in Visual Studio and build/run `application_executable` (outputs `LastFrame.exe`). The startup project is set to `application_executable` by default.

**Build from CLI:**
```bash
cd build/VisualStudio
cmake --build . --target application_executable --config Debug
```

**Run tests:**
```bat
Tester.bat
```
Or manually:
```bash
cd build/VisualStudio
cmake --build . --target tester_executable --config Debug
ctest
```

**Important build notes:**
- Debug builds force `_ITERATOR_DEBUG_LEVEL=0` to stay compatible with the precompiled Jolt physics library.
- `Engine/Scripting/Script.cpp` and `Application/Reflection/ReflectionRegister.cpp` require `/bigobj` due to size.
- The precompiled header is `Application/Utility/pch.hpp` — include it first in every translation unit.
- Runtime assets live in `Extern/` and are referenced relative to the executable at runtime.

## Architecture

### Project Structure

```
Engine/         # Static library (engine_library / EngineLibrary.lib)
Application/    # Executable entry point + game-specific code (LastFrame.exe)
Lib/            # External library source (Jolt, etc.)
Extern/         # Runtime assets (scenes, scripts, meshes, audio, shaders, etc.)
Tester/         # GoogleTest unit/integration tests (tester_executable)
```

> **Note:** Due to historical configuration, `Application/` contains both game-specific and some engine-like code (e.g., `Reflection/`, `Assets/`). All `.cpp`/`.hpp` files under both `Engine/` and `Application/` (except `Application/win_main.cpp`) are compiled into `engine_library`. Only `Application/win_main.cpp` goes into `application_executable`.

### Core ECS Framework (`Engine/ECS/`)

- **`CoreEngine`** (`Engine/ECS/System/CoreEngine.hpp`) — Singleton that owns and drives all game systems. Systems are registered with `AddSystem()`, each assigned independent `init_order`, `update_order`, and `destruct_order` priorities. Each frame calls `UpdateSystems()` then `PostUpdateSystems()`.
- **`ISystem`** (`Engine/ECS/System/ISystem.hpp`) — Interface all game systems implement: `Initialize()`, `Update(float)`, and optionally `PostUpdate(float)`. `CoreEngine` is a `friend` and is the only caller.
- **Entity registry** — Uses [EnTT](https://github.com/skypjack/entt). Access via `CoreEngine::GetInstance().GetRegistry()`.
- Systems use a pseudo-singleton pattern: store `this` in a static pointer during construction for global access, but creation is managed exclusively by `CoreEngine`.

### Engine Subsystems

| Directory | Purpose |
|-----------|---------|
| `Engine/ECS/Components/` | All ECS components (Transform, Mesh, Physics, Audio, Camera, UI, etc.) |
| `Engine/Graphics/` | Renderer, shaders, meshes, camera, animation, instancing, debug drawing |
| `Engine/Physics/` | Jolt physics backend (`BackendJolt/`) and physics runtime system |
| `Engine/Audio/` | FMOD audio wrapper (`Audio.cpp/hpp`) |
| `Engine/Scripting/` | Lua scripting via sol2 (`Script.cpp/hpp`, `ScriptWrapper`) |
| `Engine/Input/` | Input manager with key mapping and action system |
| `Engine/Spatial/` | BVH spatial structures |
| `Engine/UI/` | UI system and button components |
| `Engine/LevelEditor/` | ImGui-based level editor (Windows debug only) |
| `Engine/Diagnostics/` | Engine profiler |
| `Engine/Math/` | Math utilities (wraps GLM) |
| `Engine/Assets/` | Asset loading infrastructure (loaders, scripting integration) |
| `Application/Reflection/` | Reflection registration (`ReflectionRegister.cpp`) — registers all component types |
| `Application/Events/` | Event structures (`event_structures.hpp`) |

### Asset System (`Engine/Assets/System/AssetManager.hpp`)

Assets use a GUID-based system:
- `instance_guid` — stable per-asset identifier, persisted in `guid.meta` files within each asset folder.
- `type_guid` — derived from asset type name.
- `full_guid` — combination of both, used as the key in `resource_map`.
- Components store `xresource::full_guid` (or just `instance_guid` for serialization), **never raw pointers**.
- `Extern/Config.json` maps asset type names to their folders. The AssetManager reads this at init and calls per-type loader functions.

### Scene / Serialization

Scenes are JSON files in `Extern/Scenes/`. The active scene is hardcoded as `M4_Level.json` (see `Application/main.cpp`). Overlays (UI screens) are JSON files in `Extern/Overlays/`. Archetypes (entity templates) live in `Extern/Archtypes/`.

### Scripting

Game logic is authored in Lua (`Extern/Scripts/*.lua`) via the sol2 binding layer. Scripts are loaded as an asset type and attached to entities via script components. The `ENGINE_ENABLE_LUA` preprocessor define gates Lua support.

### External Libraries

Key libs (linked via `Lib/` or prebuilt in `Lib/Windows_x64/`):
- **EnTT** — ECS registry
- **GLM** — math
- **Jolt** — physics (built from source in `Lib/`)
- **FMOD** — audio (prebuilt DLL, copied post-build)
- **GLAD / GLFW / OpenGL** — rendering
- **ImGui** — level editor UI (Debug only)
- **sol2 + Lua** — scripting
- **Assimp / OpenFBX** — mesh import
- **nlohmann/json** — JSON serialization
- **stb_image** — texture loading
- **GoogleTest** — unit testing
