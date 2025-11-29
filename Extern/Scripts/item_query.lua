----------------------------------------------------------------
-- item_query.lua
----------------------------------------------------------------

local REQUIRED_ITEM_ID = 1        
local PLAYER_ENTITY_ID = 5        
local INTERACT_RANGE = 80.0        
local FACING_THRESHOLD = 0.5      -- Higher = must look more directly

function on_create(Self)
    print("[ItemQuery] Created for entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
local function IsPlayerInRange()
    local playerTransform = GetEntityTransform(PLAYER_ENTITY_ID)
    if not playerTransform then return false end

    local px, py, pz = playerTransform:GetPosition()
    local ix, iy, iz = EntityTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    return dist <= INTERACT_RANGE, px, py, pz, ix, iy, iz
end

----------------------------------------------------------------
-- Facing check (player must look at item)
----------------------------------------------------------------
local function IsPlayerFacingItem(px, py, pz, ix, iy, iz)
    -- Get camera (actual player facing)
    local cam = GetMainCamera()
    if not cam then return false end

    -- Camera forward
    local cx, cy, cz = cam:GetPosition()
    local tx, ty, tz = cam:GetTarget()

    local fwdX = tx - cx
    local fwdY = ty - cy
    local fwdZ = tz - cz

    -- normalize
    local magF = math.sqrt(fwdX*fwdX + fwdY*fwdY + fwdZ*fwdZ)
    fwdX = fwdX / magF
    fwdY = fwdY / magF
    fwdZ = fwdZ / magF

    -- direction player → item
    local dirX = ix - px
    local dirY = iy - py
    local dirZ = iz - pz

    local magD = math.sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ)
    dirX = dirX / magD
    dirY = dirY / magD
    dirZ = dirZ / magD

    -- Dot product
    local dot = fwdX * dirX + fwdY * dirY + fwdZ * dirZ

    return dot > FACING_THRESHOLD
end


----------------------------------------------------------------
-- Required item check
----------------------------------------------------------------
local function PlayerHasKey()
    local inv = SHARED["player_inventory_ref"]
    if not inv then return false end

    local item = inv.items[REQUIRED_ITEM_ID]
    return item and item.qty > 0
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    -- Distance check
    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end

    -- Facing check
    if not IsPlayerFacingItem(px, py, pz, ix, iy, iz) then
        -- print("[ItemQuery] Player near but NOT facing item")
        return
    end

    -- Now both conditions are true:
    -- ✔ close enough
    -- ✔ looking at item

    if PlayerHasKey() then
        print("[ItemQuery] Player HAS key → SHOW UNLOCK ICON")
    else
        print("[ItemQuery] Player missing key → SHOW LOCK ICON")
    end
end
