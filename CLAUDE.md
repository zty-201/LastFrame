# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Configure (run once, or after CMakeLists changes):**
```
Configure_VisualStudio_No_Analysis.bat   # faster, no ASAN
```
Then open `build/VisualStudio/LastFrame.sln` and build the `application_executable` target (outputs as `LastFrame.exe`). The VS startup project is already set to `application_executable`.

**Tests:**
```bat
Tester.bat            # configure + build + run tests (Debug & Release)
ManualTester.bat      # same but skips reconfigure if build/ exists
```
Or manually from `build/VisualStudio/`:
```
cmake --build . --target tester_executable --config Debug
ctest -C Debug -R <TestName>   # run a single test by name
```
Tests use GoogleTest and live in `Tester/`. The only current test file is `Tester/ExampleTest.cpp`.

**CMake preset used:** `visual_studio_x64_no_analyze`

## Architecture

### Compile targets
- **`engine_library`** (static lib) — all source in `Engine/` and `Application/` *except* `Application/win_main.cpp`. This includes Reflection and Asset management code that is historically misplaced in `Application/`.
- **`application_executable`** — only `Application/win_main.cpp`, links `engine_library`.
- **`tester_executable`** — `Tester/` sources + `engine_library` + GTest.

PCH: `Application/Utility/pch.hpp` (applied to `engine_library`; include it in `.cpp` files, not headers).

### System loop
`CoreEngine` (singleton, `Engine/ECS/System/CoreEngine.hpp`) owns all `ISystem` subclasses. Systems are registered with explicit `init_order`, `update_order`, `destruct_order` chars. Each frame: `UpdateSystems()` then `PostUpdateSystems()`. Systems implement `Initialize()`, `Update(float)`, and optionally `PostUpdate(float)` — all private, accessible only to `CoreEngine` via friendship.

The pseudo-singleton pattern for systems: store `this` in a static pointer in the constructor; expose a `static GetInstance()` that returns it.

Global ECS registry and event dispatcher: `extern entt::registry registry` and `extern entt::dispatcher event_dispatcher` declared in `Application/Utility/global.hpp`.

### Asset system
`AssetManagerSystem` (`Engine/Assets/System/AssetManager.hpp`) loads assets at startup from `Extern/Config.json` (`ASSET_CONFIG_FILE`). Each asset type has a loader function registered in `asset_loader_map`. Assets are stored as `entt::meta_any` in `resource_map` keyed by `xresource::full_guid`.

- **GUIDs**: `instance_guid` is generated once and persisted in `guid.meta` files alongside asset folders. `type_guid` is derived from the asset type name string. `full_guid = type_guid + instance_guid`.
- **Components store GUIDs, not pointers.** Use `AssetManagerSystem::GetResource<T>(guid)` to get a `shared_ptr<T>` at use time.
- Asset type string constants are on `AssetManagerSystem`: `TEXTURE`, `MESH`, `SHADER`, `CUBEMAP`, `AUDIO`, `SCRIPT`, `ARCHTYPE`, `ANIMATION`, `VIDEO`.

### Scripting
Lua scripts live in `Extern/Scripts/` and are loaded as `ScriptAsset`. Each entity can have a `LuaScriptComponent` containing multiple `LuaInstance`s (one per script GUID). The `ScriptSystem` (`Engine/Scripting/Script.hpp`) calls `OnCreate`, `OnUpdate(dt)`, `OnDestroy` on each instance's `sol::environment`. Scripts are toggled by `ENGINE_ENABLE_LUA` preprocessor define (always on for engine builds).

### Reflection & serialization
`RegisterReflection()` (`Application/Reflection/ReflectionRegister.cpp`) registers all components into EnTT's meta system at startup. Serialization/deserialization uses `SetJsonWithMetaAny()` / `SetMetaAnyWithJson()`. Components tagged `REGISTERED_TYPE::COMPONENT` participate in scene serialization. `ReflectionRegister.cpp` requires `/bigobj` (set in CMake).

### Scene & state machine
- Active scene: `Extern/Scenes/M4_Level.json`. Loaded by the level editor or runtime on startup.
- High-level game flow uses a `StateMachine` over `IState` subclasses (`Engine/States/`): `SplashState`, `MainMenuState`, `IntroCutsceneState`, `GameplayState`.
- `CoreEngine::RequestSceneRestart()` / `RequestQuit()` / `SetPaused()` are the safe cross-system signals.

### Overlays / UI
JSON overlay definitions live in `Extern/Overlays/`. The UI system (`Engine/UI/UISystem.hpp`) reads these to build in-game UI. Button events are dispatched through `event_dispatcher` and caught by `ScriptSystem::OnButtonEvent`.

### Key constraints
- `_ITERATOR_DEBUG_LEVEL=0` is forced in Debug/RelWithDebInfo to match Jolt's precompiled lib — do not change this.
- Jolt uses dynamic CRT (`/MD`/`/MDd`); keep all targets consistent.
- `Script.cpp` and `ReflectionRegister.cpp` require `/bigobj`; already set in CMake.
- Android builds compile everything into a single `jnicpp` shared library; assets are copied to `Android/app/src/main/assets/Extern/` at configure time.
