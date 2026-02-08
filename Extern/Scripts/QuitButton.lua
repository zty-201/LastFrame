print("[QuitButton] SCRIPT LOADED")

function on_button_Quit(self)
    print("[QuitButton] Quit clicked -> StartMenu")
    UI_CloseAllMenus()
    UI_DisplayMenu("StartMenu")
end
