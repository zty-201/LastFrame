----------------------------------------------------------------
-- key_interaction.lua
-- The simplest possible interaction detector.
-- When the player looks at this object, it returns "key".
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE = 120.0
local FACING_THRESHOLD = 0.5

function on_create(self)
    print("[KeyInteraction] Ready on entity:", EntityID)
end


-- Distance
local function IsPlayerInRange()
    local pt = GetEntityTransform(PLAYER_ENTITY_ID)
    if not pt then return false end

    local px,py,pz = pt:GetPosition()
    local ix,iy,iz = EntityTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz

    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
    return dist <= INTERACT_RANGE, dist
end


-- Facing
local function IsFacing()
    local cam = GetMainCamera()
    if not cam then return false end

    local cx,cy,cz = cam:GetPosition()
    local tx,ty,tz = cam:GetTarget()

    local fx,fy,fz = tx-cx, ty-cy, tz-cz
    local fm = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx,fy,fz = fx/fm, fy/fm, fz/fm

    local ix,iy,iz = EntityTransform:GetPosition()
    local px,py,pz = cam:GetPosition()

    local dx,dy,dz = ix-px, iy-py, iz-pz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    local dot = fx*dx + fy*dy + fz*dz
    return dot > FACING_THRESHOLD
end


----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(self, dt)

    local inRange, dist = IsPlayerInRange()
    if not inRange then return end

    if not IsFacing() then return end

    ------------------------------------------------------------
    -- HARD CODED RETURN VALUE
    ------------------------------------------------------------
    SHARED.interactable      = EntityID
    SHARED.interactableType  = "key"
    SHARED.interactableDist  = dist

    print("[KeyInteraction] Player looking at KEY, dist=", dist)
end
