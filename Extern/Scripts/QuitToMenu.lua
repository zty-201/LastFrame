-- QuitButton.lua
-- For now: "Quit" returns to StartMenu (not exiting app)

print("[QuitButton] SCRIPT LOADED")

local function set_ui_nav_lock(seconds)
    _G.__UI_NAV_LOCK = seconds or 0.25
end

function on_create(self)
    print("[QuitButton] on_create")
end

-- ButtonComponent.event_trigger MUST be "QuitButton"
function on_button_QuitToMenu(self)
    print("[QuitButton] Returning to Start Menu")

    SHARED = SHARED or {}

    -- Reset shared UI state
    SHARED.UI_INVENTORY_OPEN = false
    SHARED.UI_INVENTORY_ANDROID_STATE = false

    -- Reset cutscene / ambience handoff state so a fresh Play works next time
    SHARED.CUTSCENE_AUDIO_COMMAND = "STOP"
    SHARED.CUTSCENE_AMBIENCE = nil
    SHARED.GAMEPLAY_BASE_AMBIENCE = nil

    _G.GAME_STATE = "MENU"
    _G.INDOOR_COUNT = 0
    _G.ROOFTOP_COUNT = 0
    _G.ACTIVE_AMBIENCE_SLOT = nil
    _G.__UI_NAV_LOCK = 0.25

    -- Do NOT close all menus here, otherwise gameplay becomes visible briefly.
    -- Leave the pause screen up while the state machine transitions/restarts.
    StateMachine_SetTrigger("ReturnToMenu")
    Scene_RequestRestart()
end

function on_destroy(self)
    print("[QuitButton] on_destroy")
end
