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
end

-- IMPORTANT: C++ builds handlerName = "on_button_" + e.Event_Name
-- So if Button_Event.Event_Name == "PlayButton", this must be on_button_PlayButton(self)
function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    if self.cutscene_started then
        print("[PlayButtonController] Cutscene already playing, ignoring")
        return
    end

    self.cutscene_started = true

    local ok, err = pcall(function()
        UI_StartPseudoCutscene()
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
