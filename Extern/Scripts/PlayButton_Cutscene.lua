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

    if StateMachine_SetTrigger == nil then
        print("[PlayButtonController] ERROR: StateMachine_SetTrigger is NIL (binding missing)")
    else
        print("[PlayButtonController] StateMachine_SetTrigger exists")
    end
end

function on_update(self, dt)
    -- No cutscene logic here anymore.
end

function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    _G.GAME_STATE = "GAME"
    _G.INDOOR_COUNT = 0

    if StateMachine_SetTrigger == nil then
        print("[PlayButtonController] ERROR: StateMachine_SetTrigger binding missing")
        return
    end

    local ok, err = pcall(function()
        StateMachine_SetTrigger("StartIntro")
    end)

    if ok then
        print("[PlayButtonController] Requested MainMenu -> IntroCutscene")
    else
        print("[PlayButtonController] ERROR triggering StartIntro: " .. tostring(err))
    end
end

function on_destroy(self)
    print("[PlayButtonController] Destroyed. EntityID=" .. tostring(EntityID))
end

-- IMPORTANT: C++ builds handlerName = "on_button_" + e.Event_Name
-- So if Button_Event.Event_Name == "PlayButton", this must be on_button_PlayButton(self)
function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    _G.GAME_STATE = "GAME"
    _G.INDOOR_COUNT = 0

    if StateMachine_SetTrigger == nil then
        print("[PlayButtonController] ERROR: StateMachine_SetTrigger binding missing")
        return
    end

    local ok, err = pcall(function()
        StateMachine_SetTrigger("StartIntro")
    end)

    if ok then
        print("[PlayButtonController] Requested MainMenu -> IntroCutscene")
    else
        print("[PlayButtonController] ERROR triggering StartIntro: " .. tostring(err))
    end
end

-- OPTIONAL: If you make another button with event_trigger="ReloadPlayScript",
-- it will call this and reload the script
function on_button_ReloadPlayScript(self)
    print("[PlayButtonController] ===== RELOAD BUTTON PRESSED =====")
    try_reload_self("button ReloadPlayScript")
end