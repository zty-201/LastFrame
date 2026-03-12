# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Configure (do once, or after CMakeLists changes):**
```
Configure_VisualStudio_No_Analysis.bat   # faster, no ASAN — preferred
Configure_VisualStudio.bat               # with clang-tidy + ASAN
```
Both generate `build/VisualStudio/LastFrame.sln` (Visual Studio 2022, x64).

**Build:**
Open the `.sln` and build `application_executable`, or from the build directory:
```
cmake --build . --target application_executable --config Debug
```

**Tests:**
```
Tester.bat
# or manually:
cmake --build . --target tester_executable --config Debug && ctest
```
Tests live in `Tester/` and use GoogleTest.

## Architecture

### Directory Layout
- `Engine/` — reusable engine systems, all compiled into `engine_library` static lib
- `Application/` — game-specific code; also compiles into `engine_library` except `win_main.cpp`
- `Extern/` — runtime assets: scenes, scripts, audio, meshes, shaders, overlays
- `Lib/` — precompiled third-party dependencies (Jolt, FMOD, EnTT, sol2, etc.)

> `Application/` holds Reflection and Asset code for historical reasons — conceptually treat it as engine code.

### ECS (EnTT)
- **`CoreEngine`** (singleton, `Engine/ECS/System/CoreEngine.hpp`) owns all systems and the EnTT `registry`.
- Systems inherit `ISystem` (`Engine/ECS/System/ISystem.hpp`) and implement `Initialize()`, `Update(dt)`, and optionally `PostUpdate(dt)`.
- Systems are registered in `Application/main.cpp` (`Game_Initialize`) with explicit init/update/destruct ordering integers.
- Global `registry` and `event_dispatcher` are declared in `Application/Utility/global.hpp`.

**System update order (lower = earlier):**
`InputManager(1) → StateMachineSystem(2) → ScriptWrapper(3) → PhysicsRuntimeSystem(4) → AssetManager(5) → GameObjectSystem(6) → UI_System(7) → AnimationSystem(8) → GraphicsSystem(9) → AudioSystem(10)`

### Scripting (Lua / sol2)
- Lua scripts live in `Extern/Scripts/`; entities reference them via `LuaScriptComponent`.
- Script lifecycle: `OnCreate()`, `OnUpdate(dt)`, `OnDestroy()`.
- Native C++ scripts use `NativeScriptComponent` with the same lifecycle.
- `ScriptWrapper` system drives both.

### Asset & Serialization System
- Assets are identified by stable GUIDs stored in `guid.meta` files alongside the assets.
- Components store GUIDs, never raw pointers to assets.
- Asset manifest: `Extern/Config.json`.
- All serializable components must be registered in `Application/Reflection/ReflectionRegister.cpp` using EnTT meta-reflection (`entt::meta<T>()`).
- The active scene is hardcoded to `Extern/Scenes/M4_Level.json`.

### Event System
- EnTT dispatcher (`event_dispatcher` in `global.hpp`) is used for broadcasting events.
- Custom event types: `Event_OnTriggerEnter`, `Event_OnTriggerExit`, `Button_Event`, etc.

### Platform
- `Application/win_main.cpp` — Windows entry point.
- Android builds produce `jnicpp.so`; input uses overlay JSON files in `Extern/Overlays/`.

## Key Build Constraints
- `_ITERATOR_DEBUG_LEVEL=0` is forced in Debug to stay compatible with Jolt's precompiled libraries — do not change this.
- `Script.cpp` and `ReflectionRegister.cpp` require `/bigobj` (already set in CMake).
- Precompiled header: `Application/Utility/pch.hpp` — include common headers there, not in individual `.cpp` files.
- Jolt is built with RTTI and exceptions disabled; don't enable them project-wide.
