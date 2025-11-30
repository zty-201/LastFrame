----------------------------------------------------------------
-- door_query_debug.lua
----------------------------------------------------------------

local REQUIRED_ITEM_ID  = "key"
local PLAYER_ENTITY_ID  = 5
local INTERACT_RANGE    = 80.0
local FACING_THRESHOLD  = 0.5

local DEBUG = true

function on_create(Self)
    print("[DoorQuery] Created for door entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check (debug-enabled)
----------------------------------------------------------------
local function IsPlayerInRange()
    -- ALWAYS fetch door's transform by ID, not by context
    local doorID = EntityID
    local doorTransform = GetEntityTransform(doorID)

    if not doorTransform then
        print("[DoorQuery] ERROR: DOOR TRANSFORM NIL for", doorID)
        return false
    end

    local playerTransform = GetEntityTransform(PLAYER_ENTITY_ID)
    if not playerTransform then return false end

    local px, py, pz = playerTransform:GetPosition()
    local ix, iy, iz = doorTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    -- print("[DoorQuery] FINAL DIST =", dist)

    return dist <= INTERACT_RANGE, px, py, pz, ix, iy, iz
end



----------------------------------------------------------------
-- Facing check (debug)
----------------------------------------------------------------
local function IsPlayerFacingDoor(px, py, pz, ix, iy, iz)
    local cam = GetMainCamera()
    if not cam then 
        print("[DoorQuery] ERROR: Camera is NIL")
        return false 
    end

    local cx, cy, cz = cam:GetPosition()
    local tx, ty, tz = cam:GetTarget()

    local fx, fy, fz = tx - cx, ty - cy, tz - cz
    local fm = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx, fy, fz = fx/fm, fy/fm, fz/fm

    local dx = ix - px
    local dy = iy - py
    local dz = iz - pz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx, dy, dz = dx/dm, dy/dm, dz/dm

    local dot = fx*dx + fy*dy + fz*dz

    if DEBUG then
        print(string.format("[DoorQuery] FacingDot = %.3f (threshold %.3f)", dot, FACING_THRESHOLD))
    end

    return dot > FACING_THRESHOLD
end

----------------------------------------------------------------
-- Required item check (debug)
----------------------------------------------------------------
local function PlayerHasKey()
    if DEBUG then
        print("[DoorQuery] Checking inventory...")
    end

    local inv = SHARED["player_inventory_ref"]
    if not inv then 
        print("[DoorQuery] ERROR: player_inventory_ref is NIL")
        return false 
    end

    local item = inv.items[REQUIRED_ITEM_ID]

    if DEBUG then
        if item then
            print("[DoorQuery] KEY FOUND qty =", item.qty)
        else
            print("[DoorQuery] NO KEY FOUND")
        end
    end

    return item and item.qty > 0
end

----------------------------------------------------------------
-- on_update (with debug)
----------------------------------------------------------------
function on_update(Self, dt)

    --- Force this script's entity context without changing anything
    EntityTransform:GetPosition()

    -- Check range
    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then 
        return 
    end

    -- Check facing
    if not IsPlayerFacingDoor(px, py, pz, ix, iy, iz) then
        return
    end

    -- Check key
    if PlayerHasKey() then
        print("[DoorQuery] >>> UNLOCK <<<")
        SHARED.door_icon_status = "unlock"
    else
        print("[DoorQuery] >>> LOCK <<<")
        SHARED.door_icon_status = "lock"
    end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "door"
    SHARED.interactableDist = 0
end
