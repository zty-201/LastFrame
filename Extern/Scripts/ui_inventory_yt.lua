-- UI_Inventory.lua
-- Inventory manager / panel controller

local DEBUG = true

local inventoryOpen = false
local prevTogglePressed = false

-- Layout settings (NDC)
local START_X   = -0.56
local START_Y   = -0.8
local SPACING_X =  0.28



local function debug_print(...)
    if DEBUG then
        print("[UI_Inventory]", ...)
    end
end

local function print_inventory_state()
    debug_print("===== INVENTORY STATE =====")

    debug_print("ORDER:")
    for i = 1, #SHARED.UI_INVENTORY_ORDER do
        debug_print("  [" .. i .. "] = " .. tostring(SHARED.UI_INVENTORY_ORDER[i]))
    end

    debug_print("ICONS:")
    for k, v in pairs(SHARED.UI_INVENTORY_ICONS) do
        debug_print(
            "  key = " .. tostring(k) ..
            ", item_id = " .. tostring(v.item_id) ..
            ", ndc_x = " .. tostring(v.ndc_x) ..
            ", ndc_y = " .. tostring(v.ndc_y)
        )
    end

    debug_print("===========================")
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inventory] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function is_toggle_pressed()
    return Input.GetAction("INVENTORY")
end

local function reset_inventory_state()
    SHARED.UI_INVENTORY = {}
    SHARED.UI_INVENTORY_OPEN = false
    SHARED.UI_INVENTORY_ICONS = {}
    SHARED.UI_INVENTORY_ORDER = {}

    inventoryOpen = false
    prevTogglePressed = false

    debug_print("Inventory state reset")
end

local function ensure_tables()
    SHARED.UI_INVENTORY = SHARED.UI_INVENTORY or {}
    SHARED.UI_INVENTORY_OPEN = SHARED.UI_INVENTORY_OPEN or false
    SHARED.UI_INVENTORY_ICONS = SHARED.UI_INVENTORY_ICONS or {}
    SHARED.UI_INVENTORY_ORDER = SHARED.UI_INVENTORY_ORDER or {}
end

local function exists_in_order(entityId)
    local order = SHARED.UI_INVENTORY_ORDER
    for i = 1, #order do
        if order[i] == entityId then
            return true
        end
    end
    return false
end

local function remove_from_order(entityId)
    local order = SHARED.UI_INVENTORY_ORDER
    for i = #order, 1, -1 do
        if order[i] == entityId then
            table.remove(order, i)
        end
    end
end

local function recalculate_positions()
    local order = SHARED.UI_INVENTORY_ORDER
    local icons = SHARED.UI_INVENTORY_ICONS

    local visibleIndex = 0

    for i = 1, #order do
        local entityId = order[i]
        local iconData = icons[entityId]

        if iconData ~= nil then
            iconData.ndc_x = START_X + visibleIndex * SPACING_X
            iconData.ndc_y = START_Y

            debug_print(
                "Recalculated:",
                "entity =", tostring(entityId),
                "slot =", tostring(visibleIndex),
                "x =", tostring(iconData.ndc_x),
                "y =", tostring(iconData.ndc_y)
            )

            visibleIndex = visibleIndex + 1
        else
            debug_print("Skipped stale order entry:", tostring(entityId))
        end
    end
end

function on_create(self)
    
    reset_inventory_state()
    ensure_tables()

    ------------------------------------------------
    -- Add icon
    ------------------------------------------------
    function SHARED.UI_INVENTORY.AddIcon(entityId, itemId)
        ensure_tables()

        if entityId == nil then
            debug_print("AddIcon failed: entityId is nil")
            return
        end

        if SHARED.UI_INVENTORY_ICONS[entityId] ~= nil then
            debug_print("AddIcon skipped: icon already exists for entity", tostring(entityId))
            return
        end

        SHARED.UI_INVENTORY_ICONS[entityId] = {
            entity_id = entityId,
            item_id = itemId,
            ndc_x = 0.0,
            ndc_y = 0.0
        }

        if not exists_in_order(entityId) then
            table.insert(SHARED.UI_INVENTORY_ORDER, entityId)
        else
            debug_print("AddIcon warning: entity already existed in order list", tostring(entityId))
        end

        debug_print("Added icon:", tostring(entityId), tostring(itemId))
        recalculate_positions()
        --print_inventory_state()
    end

    ------------------------------------------------
    -- Remove icon
    ------------------------------------------------
    function SHARED.UI_INVENTORY.RemoveIcon(entityId, itemId)
        ensure_tables()

        if entityId == nil then
            debug_print("RemoveIcon failed: entityId is nil")
            return
        end

        SHARED.UI_INVENTORY_ICONS[entityId] = nil
        remove_from_order(entityId)

        debug_print("Removed icon:", tostring(entityId), tostring(itemId))
        recalculate_positions()
        print_inventory_state()
    end

    ------------------------------------------------
    -- Recalculate positions
    ------------------------------------------------
    function SHARED.UI_INVENTORY.RecalculatePositions()
        ensure_tables()
        recalculate_positions()
    end

    ------------------------------------------------
    -- Get stored position for one icon entity
    ------------------------------------------------
    function SHARED.UI_INVENTORY.GetPosition(entityId)
        ensure_tables()

        local data = SHARED.UI_INVENTORY_ICONS[entityId]
        if data == nil then
            return nil, nil
        end

        return data.ndc_x, data.ndc_y
    end

    SHARED.UI_INVENTORY_OPEN = false
    inventoryOpen = false

    set_visible(false)
    debug_print("Initialized")
end

function on_update(self, dt)

    --UI_DisplayMenu("Android_Inventory_Buttons")
    ensure_tables()

    

    -- Android state control
    -- Android state control
    if PLATFORM == "windows" then
        if SHARED.UI_INVENTORY_ANDROID_STATE ~= nil then
            if SHARED.UI_INVENTORY_ANDROID_STATE ~= inventoryOpen then
                inventoryOpen = SHARED.UI_INVENTORY_ANDROID_STATE
                SHARED.UI_INVENTORY_OPEN = inventoryOpen

                set_visible(inventoryOpen)

                if inventoryOpen then
                    SHARED.UI_INVENTORY.RecalculatePositions()
                    debug_print("Inventory opened")
                else
                    debug_print("Inventory closed")
                end

                if SHARED.UI_INVENTORY_ANDROID_STATE then
                    debug_print("android_inventory_open_state")
                else
                    debug_print("android_inventory_close_state")
                end
            end
        end
    end
    -- PC control
    -- elseif PLATFORM == "windows" then
    --     local pressed = is_toggle_pressed()
    --     if pressed and not prevTogglePressed then
    --         inventoryOpen = not inventoryOpen
    --         SHARED.UI_INVENTORY_OPEN = inventoryOpen
    --         set_visible(inventoryOpen)
    --         if inventoryOpen then
    --             SHARED.UI_INVENTORY.RecalculatePositions()
    --             debug_print("Inventory opened")
                
    --         else
    --             debug_print("Inventory closed")
    --         end
    --     end

    --     prevTogglePressed = pressed
    -- end
end

function on_destroy(self)
    if SHARED == nil then
        return
    end

    -- Clear inventory icons
    if SHARED.UI_INVENTORY_ICONS ~= nil then
        for k in pairs(SHARED.UI_INVENTORY_ICONS) do
            SHARED.UI_INVENTORY_ICONS[k] = nil
        end
    end

    -- Clear inventory order
    if SHARED.UI_INVENTORY_ORDER ~= nil then
        for i = #SHARED.UI_INVENTORY_ORDER, 1, -1 do
            SHARED.UI_INVENTORY_ORDER[i] = nil
        end
    end

    -- Reset state
    SHARED.UI_INVENTORY_OPEN = false
    SHARED.UI_INVENTORY = nil

    debug_print("Inventory tables cleared")
end