----------------------------------------------------------------
-- Inventory Script (Stable, Clean, Diagnostic Version)
----------------------------------------------------------------

local DEBUG_UPDATE = true

----------------------------------------------------------------
-- Inventory Script
----------------------------------------------------------------

-- local helpers (avoid global collisions)
local function AddItem(Self, itemId, itemName, amount)
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

local function RemoveItem(Self, itemId, amount)
    amount = amount or 1
    local inv = Self.inventory.items

    if not inv[itemId] then
        print("[Inventory] Cannot remove:", itemId, "(not found)")
        return false
    end

    inv[itemId].qty = inv[itemId].qty - amount

    if inv[itemId].qty <= 0 then
        print("[Inventory] Removed ALL:", itemId)
        inv[itemId] = nil
    else
        print("[Inventory] Removed:", itemId, "x"..amount)
    end

    return true
end

local function GetQuantity(Self, itemId)
    local item = Self.inventory.items[itemId]
    return item and item.qty or 0
end

local function PrintInventory(Self)
    print("====== INVENTORY ======")
    for id, data in pairs(Self.inventory.items) do
        print("ID:", id, "| Name:", data.name, "| Qty:", data.qty)
    end
end

----------------------------------------------------------------
-- on_create
----------------------------------------------------------------
function on_create(Self)
    print("[Inventory] Initialized for entity:", EntityID)

    Self.inventory = { items = {} }

    SHARED.Inventory = SHARED.Inventory or {}
    SHARED.Inventory._self = Self

    function SHARED.Inventory.Add(itemId, itemName, amount)
        local s = SHARED.Inventory._self
        if not s then return end
        AddItem(s, itemId, itemName, amount)
    end

    function SHARED.Inventory.Remove(itemId, amount)
        local s = SHARED.Inventory._self
        if not s then return false end
        return RemoveItem(s, itemId, amount)
    end

    function SHARED.Inventory.Count(itemId)
        local s = SHARED.Inventory._self
        if not s then return 0 end
        return GetQuantity(s, itemId)
    end

    function SHARED.Inventory.Print()
        local s = SHARED.Inventory._self
        if not s then return end
        PrintInventory(s)
    end

    -- optional: UI can read items
    function SHARED.Inventory.GetItems()
        local s = SHARED.Inventory._self
        return (s and s.inventory and s.inventory.items) or {}
    end
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)
    if Input.GetKeyDown(84) then -- T
        PrintInventory(Self)
    end
end

----------------------------------------------------------------
function on_destroy(Self)
    print("[Inventory] on_destroy() for entity:", EntityID)

    -- optional safety: clear shared owner if this entity was the owner
    if SHARED.Inventory and SHARED.Inventory._self == Self then
        SHARED.Inventory._self = nil
    end
end
