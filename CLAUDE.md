# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Configure (run once, or after CMakeLists changes):**
```bat
Configure_VisualStudio_No_Analysis.bat   # faster, no ASAN
Configure_VisualStudio.bat               # with ASAN + code analysis
```
Both wipe the `build/` directory and re-run `cmake --preset visual_studio_x64_no_analyze` (or the analysis variant).

**Build the game** – open `build/VisualStudio/LastFrame.sln` in Visual Studio 2022, set startup project to `application_executable`, and build/run.

**Build via CLI** (from `build/VisualStudio/`):
```bat
cmake --build . --target application_executable --config Debug
cmake --build . --target application_executable --config Release
```

**Run tests:**
```bat
Tester.bat                                           # configure + build + ctest (Debug & Release)
# or manually from build/VisualStudio/:
cmake --build . --target tester_executable --config Debug
ctest -C Debug
```

## Architecture

### Project layout
- `Engine/` – reusable engine code compiled into `engine_library` (static lib)
- `Application/` – game-specific code also compiled into `engine_library`, **except** `win_main.cpp`
- `Application/win_main.cpp` – sole source of `application_executable`; just calls `Game_Initialize / Game_Update / Game_Destroy`
- `Extern/` – runtime assets (scenes, scripts, audio, meshes, shaders, overlays, fonts)
- `Tester/` – GoogleTest unit tests compiled into `tester_executable` (not shipped)
- `Lib/` – vendored third-party libraries

> The `Application/` directory contains engine-like subsystems (Reflection, Assets, Events) due to historical configuration—all of it goes into `engine_library` except `win_main.cpp`.

### CoreEngine & systems
`EngineFramework::CoreEngine` (singleton, `Engine/ECS/System/CoreEngine.hpp`) owns all gameplay systems.

Every system inherits `ISystem` and implements `Initialize()`, `Update(float)`, and optionally `PostUpdate(float)`. Systems are registered in `Application/main.cpp::Game_Initialize()` with independent init/update/destruct priority numbers. System registration order in source doesn't matter; the numeric priorities control execution order.

Current system init order (as registered in `main.cpp`):
1. GraphicsSystem (OpenGL/GLFW window)
2. InputManager
3. AudioSystem (FMOD)
4. AssetManagerSystem
5. PhysicsRuntimeSystem (Jolt)
6. GameObjectSystem (scene/entity management)
7. ScriptWrapper (Lua via sol2)
8. UI_System
9. AnimationSystem
10. LevelEditorSystem *(Debug + Windows only, init order 100)*

To add a new system: create a class deriving `ISystem`, call `core_engine.AddSystem(...)` in `Game_Initialize`, and pick init/update/destruct priorities relative to the list above.

### ECS
EnTT registry (`CoreEngine::GetRegistry()`) is the single source of entity truth. Components live under `Engine/ECS/Components/`. All asset references inside components are stored as `xresource::full_guid`—never raw pointers or `shared_ptr`. Use `AssetManagerSystem::GetResource<T>(guid)` to resolve assets at runtime.

### Asset system
`AssetManagerSystem` (`Engine/Assets/System/AssetManager.hpp`) reads `Extern/Config.json` at startup to discover asset folders. Each asset folder has a `guid.meta` file that maps asset names to stable GUIDs persisted across loads. When adding a new asset type, register a loader function in `AssetManager::Initialize()` and add an entry to `Extern/Config.json`.

**Asset rules:**
- Serialize assets as `uint64_t` (instance GUID only; type GUID is not needed).
- To rename an asset: rename the file, then update the old name→GUID entry in the relevant `guid.meta`.
- Never store `shared_ptr<T>` asset handles in components.

### Scripting
Lua scripts live in `Extern/Scripts/` (loaded as an asset type). The engine uses sol2 to expose the C++ API. `Engine::ScriptSystem` manages Lua state; `Application/Reflection/ReflectionRegister.cpp` registers all components with EnTT's meta reflection so scripts can access components by name.

`Engine/Scripting/Script.cpp` and `Application/Reflection/ReflectionRegister.cpp` require `/bigobj` (already set in CMakeLists).

### Reflection
EnTT meta reflection is used throughout. `RegisterReflection()` (`Application/Reflection/ReflectionRegister.hpp`) is called at startup before `InitializeSystems()`. All components must be registered there to be accessible to the level editor, serialization, and scripts.

### Active scene
The scene loaded at runtime is hardcoded in `Application/main.cpp`:
```cpp
static const char* g_MainSceneFile = "M4_Level.json";
```
Scenes are JSON files in `Extern/Scenes/`. Scene restart is triggered via `CoreEngine::RequestSceneRestart()`.

### Events
A global `entt::dispatcher event_dispatcher` is used for decoupled communication. Event types are declared in `Application/event_structures.hpp`. `event_dispatcher.update()` is called once per frame at the end of `Game_Update()`.

### Level editor
`LevelEditorSystem` and `imgui` are compiled in only for `_DEBUG + PLATFORM_WINDOWS`. The default ImGui dock layout is copied from `Extern/LevelEditor/imgui_default.ini` to the build output directory at configure time.

### Key build constraints
- `_ITERATOR_DEBUG_LEVEL=0` is forced in Debug to match Jolt's precompiled lib. Do not remove this.
- Use dynamic CRT (`/MD`/`/MDd`) everywhere—static CRT will cause linker conflicts with Jolt.
- PCH: `Application/Utility/pch.hpp` is the shared precompiled header for `engine_library`. Only `#include "pch.hpp"` in `.cpp` files, not headers.
- Warnings are treated as errors (`/W4 /WX` on MSVC).
