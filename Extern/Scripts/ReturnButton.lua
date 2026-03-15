-- ReturnButton.lua
-- Returns player to the MainMenu when clicked

print("[ReturnButton] SCRIPT LOADED")

function on_create(self)
    print("[ReturnButton] on_create")
end

-- IMPORTANT:
-- ButtonComponent.event_trigger MUST be "ReturnButton"
function on_button_ReturnButton(self)
    --print("[ReturnButton] Return button clicked -> Resuming Game")

    -- Close any currently open menus
    --UI_CloseAllMenus()
    --UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
    if PLATFORM == "android" then
        UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
    end

    local state_name = StateMachine_GetCurrentStateName()
    print("[Current State] state = ", state_name)  
    

    local prev_state_name = StateMachine_GetPrevStateName()
    print("[Prev State] state = ", prev_state_name)  
    


    if prev_state_name == "Gameplay" then
        --print("[prev state] Gameplay")
        StateMachine_SetTrigger("BacktoGame")
        return
    end

    if prev_state_name == "MainMenu" then
        --print("[prev state] MainMenu")
        --UI_DisplayMenu("Android_Inventory_Buttons")
        StateMachine_SetTrigger("BacktoMainMenu")
        return
    end

    -- Display Main Menu
    -- UI_DisplayMenu("StartMenu")
end

function on_destroy(self)
    print("[ReturnButton] on_destroy")
end
