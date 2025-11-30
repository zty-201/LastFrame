----------------------------------------------------------------
-- Inventory Script (Stable, Clean, Diagnostic Version)
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

    SHARED.player_inventory_ref  = Self.inventory
    SHARED.player_inventory_self = Self
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
-- Get quantity
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
-- on_update (working)
----------------------------------------------------------------
function on_update(Self, dt)

    -- Print inventory
    if Input.GetKeyDown(84) then  -- T key
        PrintInventory(Self)
    end
end

----------------------------------------------------------------
function on_destroy(Self)
    print("[Inventory] on_destroy() for entity:", EntityID)
end
