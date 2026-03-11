-- PlayButtonController.lua

print("[PlayButtonController] ===== SCRIPT FILE LOADED =====")

local function try_reload_self(reason)
    -- We cannot reload from Lua unless the engine exposes a binding.
    -- So we call it only if it exists.
    if Script_RequestReloadEntity == nil then
        print("[PlayButtonController] Reload requested (" .. tostring(reason) .. ") BUT Script_RequestReloadEntity is NIL (binding missing)")
        return false
    end

    print("[PlayButtonController] Reload requested (" .. tostring(reason) .. ") -> calling Script_RequestReloadEntity(EntityID=" .. tostring(EntityID) .. ")")
    Script_RequestReloadEntity(EntityID)
    return true
end

local lastSkipState = false
-- Persist across frames
local _skipHeldPrev = false

local function is_skip_pressed()
    local held = false

    -- Spacebar mapped to "JUMP" (this works in your engine)
    if Input ~= nil and Input.GetAction ~= nil then
        if Input.GetAction("JUMP") then
            held = true
        end
    end

    -- Mouse click / tap (IF your engine feeds touch as mouse button 0)
    if Input ~= nil and Input.GetMouseButton ~= nil then
        if Input.GetMouseButton(0) then
            held = true
        end
    end

    -- edge detect: true only on the first frame it becomes held
    local pressed = held and (not _skipHeldPrev)
    _skipHeldPrev = held
    return pressed
end

local function try_advance_cutscene_one_step()
    -- Preferred: advance by ONE scene
    local advanceFns = {
        "UI_AdvancePseudoCutscene",
        "UI_PseudoCutsceneNext",
        "UI_NextPseudoCutsceneScene",
        "UI_CutsceneNext",
    }

    for i = 1, #advanceFns do
        local name = advanceFns[i]
        local fn = rawget(_G, name)
        if type(fn) == "function" then
            local ok, err = pcall(fn)
            if ok then
                print("[PlayButtonController] Advanced cutscene by 1 via " .. name)
                return true
            else
                print("[PlayButtonController] ERROR calling " .. name .. ": " .. tostring(err))
            end
        end
    end

    -- Fallback: end cutscene completely if that's the only API available
    local skipFns = {
        "UI_SkipPseudoCutscene",
        "UI_StopPseudoCutscene",
        "UI_EndPseudoCutscene",
        "UI_ClosePseudoCutscene",
    }

    for i = 1, #skipFns do
        local name = skipFns[i]
        local fn = rawget(_G, name)
        if type(fn) == "function" then
            local ok, err = pcall(fn)
            if ok then
                print("[PlayButtonController] No advance API; skipped entire cutscene via " .. name)
                return true
            else
                print("[PlayButtonController] ERROR calling " .. name .. ": " .. tostring(err))
            end
        end
    end

    print("[PlayButtonController] No cutscene advance/skip binding found. Expose one in C++ (recommended: UI_AdvancePseudoCutscene).")
    return false
end

function on_create(self)
    print("[PlayButtonController] ===== on_create CALLED =====")
    print("[PlayButtonController] EntityID=" .. tostring(EntityID))

    self.cutscene_started = false
    self._reload_cooldown = 0.0

    -- Debug: check bindings exist
    if UI_StartPseudoCutscene == nil then
        print("[PlayButtonController] ERROR: UI_StartPseudoCutscene is NIL (binding missing)")
    else
        print("[PlayButtonController] UI_StartPseudoCutscene exists")
    end

    if Script_RequestReloadEntity == nil then
        print("[PlayButtonController] NOTE: Script_RequestReloadEntity is NIL (reload binding not exposed yet)")
    else
        print("[PlayButtonController] Script_RequestReloadEntity exists (reload supported)")
    end
end

function on_update(self, dt)
    -- Cooldown to prevent spamming reload every frame
    if self._reload_cooldown > 0.0 then
        self._reload_cooldown = self._reload_cooldown - dt
    end

    -- Press F5 to reload this entity's scripts (Windows)
    -- (GLFW_KEY_F5 is usually 294; if you already bind constants, use that instead)
    if self._reload_cooldown <= 0.0 then
        local F5 = 294
        if Input.GetKeyDown ~= nil and Input.GetKeyDown(F5) then
            if try_reload_self("F5") then
                self._reload_cooldown = 0.5
            end
        end
    end

    -- ------------------------------------------------------------
    -- CUTSCENE ADVANCE (1 scene per press)
    -- ------------------------------------------------------------
    if self.cutscene_started then
        -- If you added UI_IsCutscenePlaying, you can guard with it:
        -- if UI_IsCutscenePlaying ~= nil and not UI_IsCutscenePlaying() then return end

        if is_skip_pressed() then
            if UI_StopPseudoCutscene ~= nil then
                UI_StopPseudoCutscene()
            else
                -- fallback: at least close menus
                if UI_CloseAllMenus ~= nil then UI_CloseAllMenus() end
            end

            self.cutscene_started = false
        end
    end
end

-- IMPORTANT: C++ builds handlerName = "on_button_" + e.Event_Name
-- So if Button_Event.Event_Name == "PlayButton", this must be on_button_PlayButton(self)
function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    _G.GAME_STATE = "GAME"
    _G.INDOOR_COUNT = 0
    
    if self.cutscene_started then
        print("[PlayButtonController] Cutscene already playing, ignoring")
        return
    end

    self.cutscene_started = true

    local ok, err = pcall(function()
        UI_StartPseudoCutscene()
        --UI_DisplayMenu("Android_Inventory_Buttons")
    end)

    if ok then
        print("[PlayButtonController] Cutscene sequence started!")
    else
        print("[PlayButtonController] ERROR calling UI_StartPseudoCutscene: " .. tostring(err))
        self.cutscene_started = false
    end
end

-- OPTIONAL: If you make another button with event_trigger="ReloadPlayScript",
-- it will call this and reload the script
function on_button_ReloadPlayScript(self)
    print("[PlayButtonController] ===== RELOAD BUTTON PRESSED =====")
    try_reload_self("button ReloadPlayScript")
end

function on_destroy(self)
    print("[PlayButtonController] Destroyed. EntityID=" .. tostring(EntityID))
end
