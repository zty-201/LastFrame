local PAGE_NUM  = 4
local FOLDER_ID = 764
local UI_TABLE  = "FOLDER_UI_OPEN"
local UI = nil
local function folder_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][FOLDER_ID] == true
end
function on_create(self)
    UI = EntityUI
    SHARED.FOLDER_PAGE = SHARED.FOLDER_PAGE or 0
    UI:SetVisible(false)
end
function on_update(self, dt)
    if not UI then return end
    local page = SHARED.FOLDER_PAGE or 0
    UI:SetVisible(folder_open() and PAGE_NUM <= page)

    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then
        --UI_CloseMenu(MENU_NAME)
        UI:SetVisible(false)
        --return
    end
    
end
