----------------------------------------------------------------
-- Folder_TriggerBox.lua
-- Attach to the world folder interactable entity.
-- E key (via InteractionZone DOOR_CMD) opens / closes the folder UI.
----------------------------------------------------------------

local FOLDER_ID      = 764          -- <<< this entity's ID (SF_Evidence_Folder)
local MENU_NAME      = "FolderInterface"
local UI_TABLE       = "FOLDER_UI_OPEN"
local DOOR_CMD_TABLE = "DOOR_CMD"

local function consume_toggle(id)
    if SHARED == nil then return false end
    local t = SHARED[DOOR_CMD_TABLE]
    if t == nil then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE]       = SHARED[UI_TABLE]       or {}
    SHARED[UI_TABLE][FOLDER_ID] = false
end

function on_update(self, dt)

    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then
        UI_CloseMenu(MENU_NAME)
        return
    end
    
    -- Auto-close if player walked out of the zone while the UI was open
    if SHARED[UI_TABLE][FOLDER_ID] == true
       and not (SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[FOLDER_ID] == true) then
        if UI_CloseAllMenus ~= nil then UI_CloseAllMenus() end
        SHARED[UI_TABLE][FOLDER_ID] = false
        if PLATFORM == "android" then
            if UI_DisplayMenu_InvisibleLayer ~= nil then UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons") end
            if UI_DisplayMenu ~= nil then UI_DisplayMenu("AndroidInputOverlay") end
        end
        print("[Folder_TriggerBox] Auto-closed: player left zone")
    end

    if not consume_toggle(FOLDER_ID) then return end

    local nowOpen = not (SHARED[UI_TABLE][FOLDER_ID] == true)
    SHARED[UI_TABLE][FOLDER_ID] = nowOpen

    if nowOpen then
        if UI_DisplayMenu ~= nil then UI_DisplayMenu(MENU_NAME) end
        print("[Folder_TriggerBox] Opened")
    else
        if UI_CloseAllMenus ~= nil then UI_CloseAllMenus() end
        if PLATFORM == "android" then
            UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
            UI_DisplayMenu("AndroidInputOverlay")
        end
        print("[Folder_TriggerBox] Closed")
    end
end
