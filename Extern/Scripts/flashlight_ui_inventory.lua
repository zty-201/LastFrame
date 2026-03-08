----------------------------------------------------------------
-- flashlight_inventory.lua
-- Attach to ANY persistent entity (e.g. the player or a
-- dedicated "inventory initialiser" entity).
--
-- The flashlight is always in the player's possession —
-- no pickup interaction needed. This script simply adds it
-- to the inventory on creation so the inventory UI icon
-- (ui_inv_key_icon.lua) can find it via SHARED.Inventory.
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "flashlight_inventory"
local ITEM_NAME = "Flashlight"
local ITEM_QTY  = 1

local function debug_print(...)
    if DEBUG then
        print("[FlashlightInventory]", ...)
    end
end

----------------------------------------------------------------
-- on_create: runs once when the entity/scene loads.
-- Immediately registers the flashlight in inventory.
----------------------------------------------------------------
function on_create(Self)
    debug_print("on_create EntityID:", EntityID)

    -- Wait for Inventory API to be ready (inventory.lua must
    -- be initialised before this script runs; ensure load order).
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        debug_print("Added", ITEM_NAME, "to inventory via Inventory.Add")
    else
        -- Fallback: write directly into the legacy items table
        -- (mirrors how key_interaction.lua adds items)
        local invRef = SHARED and SHARED.player_inventory_ref
        if invRef and invRef.items then
            local items = invRef.items
            if items[ITEM_ID] then
                items[ITEM_ID].qty = items[ITEM_ID].qty + ITEM_QTY
                debug_print("Incremented legacy", ITEM_NAME, "qty =", items[ITEM_ID].qty)
            else
                items[ITEM_ID] = { name = ITEM_NAME, qty = ITEM_QTY }
                debug_print("Added legacy", ITEM_NAME, "x" .. ITEM_QTY)
            end
        else
            -- Inventory not ready yet — flag for retry in on_update
            debug_print("WARNING: Inventory not ready on create — will retry in on_update")
            SHARED._flashlight_inv_pending = true
        end
    end
end

----------------------------------------------------------------
-- on_update: only active if on_create could not reach inventory.
-- Retries once inventory is available, then disables itself.
----------------------------------------------------------------
function on_update(Self, dt)
    if not SHARED._flashlight_inv_pending then
        on_update = nil
        return
    end

    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        debug_print("Retry succeeded: added", ITEM_NAME, "via Inventory.Add")
        SHARED._flashlight_inv_pending = false
        on_update = nil
        return
    end

    local invRef = SHARED and SHARED.player_inventory_ref
    if invRef and invRef.items then
        local items = invRef.items
        if items[ITEM_ID] then
            items[ITEM_ID].qty = items[ITEM_ID].qty + ITEM_QTY
        else
            items[ITEM_ID] = { name = ITEM_NAME, qty = ITEM_QTY }
        end
        debug_print("Retry succeeded: added", ITEM_NAME, "via legacy ref")
        SHARED._flashlight_inv_pending = false
        on_update = nil
    end
end

function on_destroy(Self)
    debug_print("on_destroy EntityID:", EntityID)
    -- Flashlight is permanent — do NOT remove it from inventory here.
end