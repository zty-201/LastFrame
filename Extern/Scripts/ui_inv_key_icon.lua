-- UI_Inv_Icon.lua
-- Generic icon script attached to each 2D inventory icon
-- Entity name format: UI_Inv_Icon_<item_id>

local DEBUG = true
local is_removed = false;
local is_added = false;

local SIZE_X =  0.06
local SIZE_Y =  0.04

local function debug_print(...)
    if DEBUG then
        print("[UI_Inv_Icon]", ...)
    end
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inv_Icon] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function get_item_id()
    local name = GetEntityName(EntityID)
    --print("===============================================My entity name is: ", name)

    return GetEntityName(EntityID)
end

local function item_exists_in_inventory(itemId)

    if itemId == nil then
        return false
    end

    if SHARED.Inventory == nil or SHARED.Inventory.GetItems == nil then
        return false
    end

    local items = SHARED.Inventory.GetItems()
    if not items then
        return false
    end

    for id, item in pairs(items) do
        if id == itemId and item and (item.amount == nil or item.amount > 0) then
            print("Inventory contains:", itemId)
            return true
        end
    end

    return false
end

function on_create(self)
    set_visible(false)

    EntityUI:SetSize_NDC(SIZE_X,SIZE_Y)
    debug_print("Initialized: ", EntityID, itemId)
end

function on_update(self, dt)

    local itemId = get_item_id()

    if SHARED.UI_INVENTORY_OPEN ~= true then
        set_visible(false)
        return
    end

    --print("================= INVENTORY IS OPEN IN INDIV SCRIPT ==============================")

    if  item_exists_in_inventory(itemId) and is_removed == false then
        --print("================= yesssssssssss ==============================")
        if is_added == false then
            is_added = true
            local itemId = get_item_id()
            if SHARED.UI_INVENTORY and SHARED.UI_INVENTORY.AddIcon then
                SHARED.UI_INVENTORY.AddIcon(EntityID, itemId)
            end
            --print("================= ITEM PICKED UP ==============================")
        end

        
        local x, y = SHARED.UI_INVENTORY.GetPosition(EntityID)
        if x ~= nil and y ~= nil then
            EntityUI:SetPosition_NDC(x, y)
            --debug_print("================= X and Y: ", x, y)
            set_visible(true)
        end
        
    else
        if is_added == true and is_removed == false then
            SHARED.UI_INVENTORY.RemoveIcon(EntityID, itemId)
            is_removed = true
            --print("================= ITEM REMOVED ==============================")
        end

        set_visible(false)
        
    end

end

function on_destroy(self)
    if SHARED.UI_INVENTORY and SHARED.UI_INVENTORY.RemoveIcon then
        SHARED.UI_INVENTORY.RemoveIcon(EntityID)
    end
end