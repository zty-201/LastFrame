# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
# Configure (faster, no static analysis):
Configure_VisualStudio_No_Analysis.bat

# Open solution and build:
build/VisualStudio/LastFrame.sln  → build target: application_executable

# Run tests:
Tester.bat
# or manually:
cmake --build . --target tester_executable --config Debug && ctest
```

- `Configure_VisualStudio.bat` enables clang-tidy and Microsoft code analysis (slower).
- C++20, MSVC `/W4 /WX` (warnings as errors). ASAN/UBSAN active in Debug.
- `_ITERATOR_DEBUG_LEVEL=0` is forced in Debug to match Jolt precompiled lib — do not remove.
- `Script.cpp` and `ReflectionRegister.cpp` require `/bigobj` (already set in CMakeLists.txt).
- PCH: `Application/Utility/pch.hpp` — include this first in all `.cpp` files.

## Architecture

### Code Layout
- `Engine/` — reusable engine systems compiled into `engine_library` static lib.
- `Application/` — game-specific code also compiled into `engine_library`, **except** `win_main.cpp` (application_executable only).
- `Tester/` — GoogleTest unit tests (Windows only).
- `Extern/` — runtime assets: scenes (JSON), Lua scripts, audio, meshes, shaders, overlays.
- `Lib/` — vendored external libraries.

### Core Loop
Entry: `Application/win_main.cpp` → `Game_Initialize / Game_Update / Game_Destroy` (defined in `Application/main.cpp`).

`Game_Update` per frame:
1. Clamp delta time to 0.1 s max.
2. `glfwPollEvents` (Windows).
3. `CoreEngine::UpdateSystems(dt)` then `PostUpdateSystems(dt)`.

### CoreEngine & ISystem
`Engine/ECS/System/CoreEngine.hpp` — singleton owning all systems as `std::unique_ptr<ISystem>`. Systems implement:
```cpp
void Initialize() override;
void Update(float delta_time) override;
void PostUpdate(float delta_time) override;
```
Systems are registered with separate `init`, `update`, and `destruct` priority values (lower = earlier). Current order (main.cpp):

| System | Init | Update | Destruct |
|---|---|---|---|
| GraphicsSystem | 1 | 9 | 8 |
| InputManager | 2 | 1 | 6 |
| StateMachineSystem | 10 | 2 | 5 |
| AudioSystem | 3 | 10 | 5 |
| AssetManager | 4 | 5 | 4 |
| PhysicsRuntimeSystem | 5 | 4 | 3 |
| GameObjectSystem | 6 | 6 | 2 |
| ScriptWrapper | 7 | 3 | 2 |
| UI_System | 8 | 7 | 3 |
| AnimationSystem | 9 | 8 | 7 |
| LevelEditorSystem | 100 | 11 | 1 (Debug/Windows only) |

### ECS
Uses **EnTT**. The central `entt::registry` is globally accessible (declared in `Application/Utility/global.hpp`). Components are plain data structs; systems query them via EnTT views. Component types include: `TransformComponent`, `MeshComponent`, `MaterialComponent`, `LightComponent`, `CameraComponent`, `PhysicsComponent`, `CharacterControllerComponent`, `ColliderComponent`, `TriggerBoxComponent`, `AudioComponent`, `AudioListenerComponent`, `AnimationComponent`, `UIComponent`, `NameComponent`, etc. Tags `SceneRenderPass` / `SkyboxRenderPass` mark entities for render passes.

### Assets
`AssetManager` system. Assets referenced by stable GUIDs stored in `.guid.meta` sidecar files. Scene JSON files live in `Extern/Scenes/`; active scene is hardcoded in `main.cpp` as `M4_Level.json`. Asset metadata index: `Extern/Config.json`.

### Scripting
Lua 5.4 via **sol2**. `ScriptWrapper` system manages script lifecycle. Scripts in `Extern/Scripts/`. C++ types exposed to Lua via `Application/Reflection/ReflectionRegister.cpp` (requires `/bigobj`).

### Platform Abstraction
`Application/Platform/Platform.h` defines `PLATFORM_WINDOWS` / `PLATFORM_ANDROID`, logging macros (`LOGI`, `LOGW`, `LOGE`, `LOGD`), and key/mouse defines for cross-platform input. Android builds output a shared lib (`jnicpp`), use OpenGL ES 3.2, and have no GLFW or ImGui.

### Key Libraries
EnTT (ECS), GLM (math), nlohmann/json, FMOD (audio), Glad+GLFW (OpenGL/window), STB Image, sol2+Lua, Jolt Physics, Assimp (OBJ/FBX only), ImGui (Debug/Windows only).

## Namespaces & Style
- Primary namespace: `EngineFramework` for engine systems.
- All `.cpp` files start with `#include "pch.hpp"`.
- Use `UNREFERENCED_PARAMETER(P)` for intentionally unused params.
- `OnScopeExit` RAII helper available in `global.hpp`.
- Platform-specific code guarded with `#ifdef PLATFORM_WINDOWS` / `#ifdef PLATFORM_ANDROID`.
