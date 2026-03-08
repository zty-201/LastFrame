
-- UI_Inventory_Android_Buttons.lua
-- Inventory manager / panel controller

local DEBUG = true


local function debug_print(...)
    if DEBUG then
        print("[UI_Inventory]", ...)
    end
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inventory] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function get_item_id()
    local name = GetEntityName(EntityID)
    --print("===============================================My entity name is: ", name)
    return GetEntityName(EntityID)
end

local function ensure_tables()
    SHARED.UI_INVENTORY_ANDROID = SHARED.UI_INVENTORY_ANDROID or {}
    if SHARED.UI_INVENTORY_ANDROID_STATE == nil then
        SHARED.UI_INVENTORY_ANDROID_STATE = false
    end
end

local function reset_tables()
    SHARED.UI_INVENTORY_ANDROID = SHARED.UI_INVENTORY_ANDROID or {}
    SHARED.UI_INVENTORY_ANDROID_STATE = false
    
end

--run when triggered by an button event. 
function on_button_OpenInventory(self)       
    SHARED.UI_INVENTORY_ANDROID_STATE = true;
    debug_print(" ================================== on_button_OpenInventory")
end

function on_create(self)

    reset_tables()
    ensure_tables()
    ------------------------------------------------
    -- Toggle Function for android
    ------------------------------------------------
    function SHARED.UI_INVENTORY_ANDROID.is_toggle_android()
        ensure_tables()
        local temp = SHARED.UI_INVENTORY_ANDROID_STATE
        SHARED.UI_INVENTORY_ANDROID_STATE = false
        return temp
    end

    

    ------------------------------------------------
    -- Pre Settings
    ------------------------------------------------

    local entity_name = get_item_id()  
    if entity_name == "android_inventory_close_state" then      
        set_visible(true)         
    end

    if entity_name == "android_inventory_open_state" then      
        set_visible(false)         
    end

    debug_print("Initialized: Android inventory buttons, on_create")
end

function on_update(self, dt)

    ensure_tables()

    local entity_name = get_item_id()

    --meaning the state is opened ah
    if entity_name == "android_inventory_open_state" then
        if SHARED.UI_INVENTORY_OPEN then
            set_visible(true)
        else
            set_visible(false)
        end      
    end

    if entity_name == "android_inventory_close_state" then
        if not SHARED.UI_INVENTORY_OPEN then
            set_visible(true)
        else
            set_visible(false)
        end      
    end

    
end

function on_destroy(self) 
    debug_print("Inventory tables cleared")
end