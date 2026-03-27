-- UI_Inventory_Android_Buttons.lua

local DEBUG = true

local function debug_print(...)
    if DEBUG then
        print("[UI_Inventory_ANDROID]", ...)
    end
end

local function getShared()
    return rawget(_G, "SHARED")
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inventory] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function get_item_id()
    return GetEntityName(EntityID)
end

local function ensure_tables()
    SHARED.UI_INVENTORY_ANDROID = SHARED.UI_INVENTORY_ANDROID or {}

    if SHARED.UI_INVENTORY_ANDROID_STATE == nil then
        SHARED.UI_INVENTORY_ANDROID_STATE = false
    end
end

------------------------------------------------
-- Button events
------------------------------------------------

function on_button_OpenInventory(self)
    ensure_tables()
    SHARED.UI_INVENTORY_ANDROID_STATE = true
    debug_print("Android inventory OPEN")
end

function on_button_CloseInventory(self)
    ensure_tables()
    SHARED.UI_INVENTORY_ANDROID_STATE = false
    debug_print("Android inventory CLOSE")
end

------------------------------------------------
-- Initialization
------------------------------------------------

function on_create(self)
    ensure_tables()
    debug_print("Android inventory buttons initialized")
end

------------------------------------------------
-- Update visibility
------------------------------------------------

function on_update(self, dt)

    if _G.GAME_STATE ~= "GAME" then
        return
    end

    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then
        return
    end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    if platform == "windows" then
        set_visible(false)
        return
    end

    --update itself is the menu is active, else return
    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        return
    end
    
    ensure_tables()

    local entity_name = get_item_id()

    if SHARED.UI_INVENTORY_ANDROID_STATE then
        --debug_print("android_inventory_open_state")
    else
        --debug_print("android_inventory_close_state")
    end

    if entity_name == "UIAndroid_InventoryOpen" then   
        if SHARED.UI_INVENTORY_ANDROID_STATE then
            set_visible(true)
        else
            set_visible(false)
        end
        
    elseif entity_name == "UIAndroid_InventoryClose" then

        if SHARED.UI_INVENTORY_ANDROID_STATE then
            set_visible(false)
            debug_print("android_inventory_close_state")
        else
            set_visible(true)
        end
    end

end

function on_destroy(self)
    debug_print("Android inventory button destroyed")
end