----------------------------------------------------------------
-- item_query.lua
-- Attach this to a door, chest, or any interactable item
----------------------------------------------------------------

-- The item wants to check this item ID in the player's inventory
local REQUIRED_ITEM_ID = 1   -- Example: key item

-- The entity ID of the player (you can also search dynamically)
local PLAYER_ENTITY_ID = nil

function on_create(Self)
    print("[ItemQuery] Created for entity:", EntityID)

    -- Find the player by checking entities with inventory
    PLAYER_ENTITY_ID = FindPlayerEntity()

    if PLAYER_ENTITY_ID then
        print("[ItemQuery] Player found:", PLAYER_ENTITY_ID)
    else
        print("[ItemQuery] ERROR: Player not found!")
    end
end

----------------------------------------------------------------
-- Helper: Find the player entity by checking who has inventory
----------------------------------------------------------------
function FindPlayerEntity()
    -- This example assumes player = 5 (your logs show player is entity 5)
    -- You can extend this to search dynamically later.
    return 5
end

----------------------------------------------------------------
-- Helper: Returns player's inventory table
----------------------------------------------------------------
function GetPlayerInventory(Self)
    if not PLAYER_ENTITY_ID then return nil end

    local playerTransform = GetEntityTransform(PLAYER_ENTITY_ID)
    if not playerTransform then return nil end

    -- Accessing the PLAYER's Lua environment:
    -- Since scripts share registry state, we can get shared values
    local player = SHARED["player_inventory_ref"]

    return player
end

----------------------------------------------------------------
-- Check if player has required key
----------------------------------------------------------------
function PlayerHasRequiredItem(Self)
    local playerInventory = SHARED["player_inventory_ref"]

    if not playerInventory then
        print("[ItemQuery] No shared player inventory reference!")
        return false
    end

    local item = playerInventory.items[REQUIRED_ITEM_ID]
    if item and item.qty > 0 then
        return true
    end

    return false
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    -- First ensure player near enough (you will add your collision check here)
    -- For now it always runs.

    local hasItem = PlayerHasRequiredItem(Self)

    if hasItem then
        -- Display unlock icon for example
        print("[ItemQuery] Player has key, SHOW UNLOCK ICON")
    else
        print("[ItemQuery] Player missing key, SHOW LOCK ICON")
    end
end

----------------------------------------------------------------
function on_destroy(Self)
    print("[ItemQuery] Destroyed for entity:", EntityID)
end
