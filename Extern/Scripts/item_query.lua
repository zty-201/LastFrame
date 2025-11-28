----------------------------------------------------------------
-- item_query.lua
-- Attach this to a door, chest, pickup, or any interactable object
----------------------------------------------------------------

local REQUIRED_ITEM_ID = 1
local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE = 30.0

local wasPlayerInRange = false
local lastHasItem = false
local wasLookedAt = false

----------------------------------------------------------------
-- Helper: Get player position
----------------------------------------------------------------
local function GetPlayerPos()
    local t = GetEntityTransform(PLAYER_ENTITY_ID)
    if not t then return nil end
    return t:GetPosition()
end

----------------------------------------------------------------
-- Helper: Get this object's position
----------------------------------------------------------------
local function GetItemPos(Self)
    local t = EntityTransform
    if not t then return nil end
    return t:GetPosition()
end

----------------------------------------------------------------
-- Range calculation
----------------------------------------------------------------
local function IsPlayerInRange(Self)
    local px, py, pz = GetPlayerPos()
    if not px then return false end

    local ix, iy, iz = GetItemPos(Self)
    if not ix then return false end

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    return dist <= INTERACT_RANGE, dist
end

----------------------------------------------------------------
-- Inventory check
----------------------------------------------------------------
local function PlayerHasRequiredItem()
    local inv = SHARED["player_inventory_ref"]
    if not inv then return false end

    local item = inv.items[REQUIRED_ITEM_ID]
    return item ~= nil and item.qty > 0
end

----------------------------------------------------------------
-- Look-at check from SHARED
----------------------------------------------------------------
local function IsPlayerLookingAtMe()
    return SHARED["look_target"] == EntityID
end

----------------------------------------------------------------
function on_create(Self)
    print("[ItemQuery] Script active on entity:", EntityID)
end

----------------------------------------------------------------
function on_update(Self, dt)

    -- 1) Distance check
    local inRange, dist = IsPlayerInRange(Self)

    if inRange ~= wasPlayerInRange then
        if inRange then
            print("[ItemQuery] Player entered range (", dist, ")")
        else
            print("[ItemQuery] Player left range")
        end
        wasPlayerInRange = inRange
    end
    
    if not inRange then return end

    -- 2) Look-at check
    local looking = IsPlayerLookingAtMe()

    if looking ~= wasLookedAt then
        if looking then
            print("[ItemQuery] Player is LOOKING at this object")
        else
            print("[ItemQuery] Player STOPPED looking at object")
        end
        wasLookedAt = looking
    end

    if not looking then return end

    -- 3) Inventory check (only when close + looking)
    local hasItem = PlayerHasRequiredItem()

    if hasItem ~= lastHasItem then
        if hasItem then
            print("[ItemQuery] Player LOOKING + IN RANGE + HAS item → SHOW UNLOCK ICON")
        else
            print("[ItemQuery] Player LOOKING + IN RANGE but MISSING item → SHOW LOCK ICON")
        end

        lastHasItem = hasItem
    end
end

----------------------------------------------------------------
function on_destroy(Self)
    print("[ItemQuery] Destroyed on entity:", EntityID)
end
