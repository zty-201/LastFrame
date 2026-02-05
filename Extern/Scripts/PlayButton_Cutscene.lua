-- PlayButtonController.lua

print("[PlayButtonController] ===== SCRIPT FILE LOADED =====")

function on_create(self)
    print("[PlayButtonController] ===== on_create CALLED =====")
    print("[PlayButtonController] EntityID=" .. tostring(EntityID))

    self.cutscene_started = false

    -- Debug: check binding exists
    if UI_StartPseudoCutscene == nil then
        print("[PlayButtonController] ERROR: UI_StartPseudoCutscene is NIL (binding missing)")
    else
        print("[PlayButtonController] UI_StartPseudoCutscene exists")
    end
end

function on_update(self, dt)
    -- Optional: minimal ping
    -- if Input.GetMouseButtonDown(0) then print("[PlayButtonController] raw click detected") end
end

-- IMPORTANT: Your C++ builds handlerName = "on_button_" + e.Event_Name
-- So if Button_Event.Event_Name == "PlayButton", this must be on_button_PlayButton(self)
function on_button_PlayButton(self)
    print("[PlayButtonController] ===== PLAY BUTTON PRESSED =====")

    if self.cutscene_started then
        print("[PlayButtonController] Cutscene already playing, ignoring")
        return
    end

    self.cutscene_started = true

    -- Start the cutscene sequence (C++ bound function)
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

function on_destroy(self)
    print("[PlayButtonController] Destroyed. EntityID=" .. tostring(EntityID))
end
