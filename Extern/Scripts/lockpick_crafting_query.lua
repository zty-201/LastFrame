----------------------------------------------------------------
-- lockpick_query.lua
-- Checks if player has ANY crafting tool (metal_pick OR allen_key)
-- Prints CRAFTABLE / UNCRAFTABLE
-- SAFE ENGINE VERSION (NO DESTROY, stable)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 80.0
local FACING_THRESHOLD = 0.5
local DEBUG            = true

-- Items that qualify as "crafting tools"
local TOOL_1 = "metal_pick"
local TOOL_2 = "allen_key"

function on_create(Self)
    print("[CraftQuery] Created for entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
local function IsPlayerInRange()
    local objectID = EntityID
    local objectTransform = GetEntityTransform(objectID)

    if not objectTransform then
        print("[CraftQuery] ERROR: Transform NIL for", objectID)
        return false
    end

    local pt = GetEntityTransform(PLAYER_ENTITY_ID)
    if not pt then return false end

    local px, py, pz = pt:GetPosition()
    local ix, iy, iz = objectTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    -- if DEBUG then print("[CraftQuery] Dist =", dist) end

    return dist <= INTERACT_RANGE, px, py, pz, ix, iy, iz
end

----------------------------------------------------------------
-- Facing check
----------------------------------------------------------------
local function IsPlayerFacing(px, py, pz, ix, iy, iz)
    local cam = GetMainCamera()
    if not cam then return false end

    local cx,cy,cz = cam:GetPosition()
    local tx,ty,tz = cam:GetTarget()

    local fx,fy,fz = tx - cx, ty - cy, tz - cz
    local fm = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx,fy,fz = fx/fm, fy/fm, fz/fm

    local dx = ix - px
    local dy = iy - py
    local dz = iz - pz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    local dot = fx*dx + fy*dy + fz*fz

    if DEBUG then
        print(string.format("[CraftQuery] FacingDot = %.3f (thresh %.3f)", dot, FACING_THRESHOLD))
    end

    return dot > FACING_THRESHOLD
end

----------------------------------------------------------------
-- Check inventory for craftable items
----------------------------------------------------------------
local function HasCraftingTool()
    local inv = SHARED["player_inventory_ref"]
    if not inv then
        print("[CraftQuery] ERROR: player_inventory_ref NIL")
        return false
    end

    local items = inv.items

    local hasPick  = items[TOOL_1] and items[TOOL_1].qty > 0
    local hasAllen = items[TOOL_2] and items[TOOL_2].qty > 0

    if DEBUG then
        print("[CraftQuery] metal_pick qty:", items[TOOL_1] and items[TOOL_1].qty or 0)
        print("[CraftQuery] allen_key qty:", items[TOOL_2] and items[TOOL_2].qty or 0)
    end

    return hasPick or hasAllen
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    -- force engine context binding (your engine pattern)
    EntityTransform:GetPosition()

    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end

    if not IsPlayerFacing(px, py, pz, ix, iy, iz) then return end

    -- Check crafting tools
    if HasCraftingTool() then
        print("[CraftQuery] >>> CRAFTABLE <<<")
        SHARED.crafting_status = "craftable"
    else
        print("[CraftQuery] >>> UNCRAFTABLE <<<")
        SHARED.crafting_status = "uncraftable"
    end

    -- mark interactable if needed
    SHARED.interactable     = EntityID
    SHARED.interactableType = "craft_station"
    SHARED.interactableDist = 0
end
