-- ReturnButton.lua
-- Returns player to the MainMenu when clicked

print("[ReturnButton] SCRIPT LOADED")

function on_create(self)
    print("[ReturnButton] on_create")
end

-- IMPORTANT:
-- ButtonComponent.event_trigger MUST be "ReturnButton"
function on_button_ReturnButton(self)
    print("[ReturnButton] Return button clicked -> Resuming Game")

    -- Close any currently open menus
    UI_CloseAllMenus()

    -- Display Main Menu
    -- UI_DisplayMenu("StartMenu")
end

function on_destroy(self)
    print("[ReturnButton] on_destroy")
end
