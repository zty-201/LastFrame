----------------------------------------------------------------
-- Inventory Script (Stable, Clean, Diagnostic Version)
----------------------------------------------------------------

----------------------------------------------------------------
-- GLOBAL DIAGNOSTIC SWITCH 
-- Set true = show update calls (only once per frame)
-- Set false = silent unless adding/removing items
----------------------------------------------------------------
local DEBUG_UPDATE = true

----------------------------------------------------------------
-- on_create
----------------------------------------------------------------
function on_create(Self)
    print("[Inventory] Initialized for entity:", EntityID)

    Self.inventory = {
        items = {}
    }

    -- Allow other scripts in the world to query player's inventory
    SHARED["player_inventory_ref"] = Self.inventory
end


----------------------------------------------------------------
-- Add item
----------------------------------------------------------------
function AddItem(Self, itemId, itemName, amount)
    amount = amount or 1
    local inv = Self.inventory.items

    if inv[itemId] then
        inv[itemId].qty = inv[itemId].qty + amount
        print("[Inventory] Added:", itemName, "x"..amount)
    else
        inv[itemId] = { name = itemName, qty = amount }
        print("[Inventory] Added new:", itemName, "x"..amount)
    end
end

----------------------------------------------------------------
-- Remove item
----------------------------------------------------------------
function RemoveItem(Self, itemId, amount)
    amount = amount or 1
    local inv = Self.inventory.items

    if not inv[itemId] then
        print("[Inventory] Cannot remove:", itemId, "(not found)")
        return false
    end

    inv[itemId].qty = inv[itemId].qty - amount

    if inv[itemId].qty <= 0 then
        print("[Inventory] Removed ALL:", inv[itemId].name)
        inv[itemId] = nil
    else
        print("[Inventory] Removed:", inv[itemId].name, "x"..amount)
    end

    return true
end

----------------------------------------------------------------
-- Get item quantity
----------------------------------------------------------------
function GetQuantity(Self, itemId)
    local item = Self.inventory.items[itemId]
    if item then return item.qty else return 0 end
end

----------------------------------------------------------------
-- Debug print
----------------------------------------------------------------
function PrintInventory(Self)
    print("====== INVENTORY ======")
    for id, data in pairs(Self.inventory.items) do
        print("ID:", id, "| Name:", data.name, "| Qty:", data.qty)
    end
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
local frame_counter = 0

function on_update(Self, dt)

    -- Add item: press E
    if Input.GetKeyDown(69) then  -- E key
        AddItem(Self, 1, "Potion", 1)
    end

    -- Remove item: press R
    if Input.GetKeyDown(82) then  -- R key
        RemoveItem(Self, 1, 1)
    end

    -- Print inventory: press T
    if Input.GetKeyDown(84) then  -- T key
        PrintInventory(Self)
    end
end


----------------------------------------------------------------
function on_destroy(Self)
    print("[Inventory] on_destroy() for entity:", EntityID)
end
