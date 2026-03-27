local function debug_print(...)
    if DEBUG then
        print("[PLAYER PICK UP]", ...)
    end
end


function on_update(self, dt)
    local item = SHARED.look_target
    if not item then return end

    -- Check if the looked-at entity is a pickup item
    if not item:HasComponent("pickup_item") then 
        return 
    end

    local comp = item:GetComponent("pickup_item")
    local id = comp.itemId
    local data = ITEMS[id]   -- from your items.lua table

    -- Player presses E
    if Input.GetKeyDown(69) then -- E key
        -- Add to inventory
        local inv = SHARED["player_inventory_ref"]
        AddItem(inv, id, data.name, 1)

        debug_print("[Pickup] Picked up:", data.name)

        -- Remove from world
        DestroyEntity(item)
    end
end
