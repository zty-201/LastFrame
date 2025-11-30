----------------------------------------------------------------
-- key_interaction.lua (SAFE VERSION)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 120.0
local FACING_THRESHOLD = 0.5

function on_create(self)
    print("[KeyInteraction] Ready on entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
local function IsPlayerInRange()
    local pt = GetEntityTransform(PLAYER_ENTITY_ID)
    if not pt then return false end

    local px, py, pz = pt:GetPosition()
    local ix, iy, iz = EntityTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz

    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
    return dist <= INTERACT_RANGE, dist
end

----------------------------------------------------------------
-- Facing check
----------------------------------------------------------------
local function IsFacing()
    local cam = GetMainCamera()
    if not cam then return false end

    local cx,cy,cz = cam:GetPosition()
    local tx,ty,tz = cam:GetTarget()

    local fx,fy,fz = tx - cx, ty - cy, tz - cz
    local fm = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx,fy,fz = fx/fm, fy/fm, fz/fm

    local ix,iy,iz = EntityTransform:GetPosition()
    local dx,dy,dz = ix - cx, iy - cy, iz - cz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    return (fx*dx + fy*dy + fz*dz) > FACING_THRESHOLD
end

----------------------------------------------------------------
-- SAFE removal method (no DestroyEntity)
----------------------------------------------------------------
local function RemoveKeySafely()
    print("[KeyInteraction] Safely removing key (NO DESTROY).")

    -- 1. Hide it instantly
    EntityTransform:SetScale(0, 0, 0)

    -- 2. Move it VERY far away so no collisions or raycasts
    local ix,iy,iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

    -- 3. Disable this script by killing update callbacks
    --    Removes on_update so engine stops calling it
    on_update = nil
    on_create = nil
    on_destroy = nil

    print("[KeyInteraction] Key entity disabled safely.")
end

----------------------------------------------------------------
-- MAIN UPDATE
----------------------------------------------------------------
function on_update(self, dt)

    local inRange, dist = IsPlayerInRange()
    if not inRange then return end
    if not IsFacing() then return end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "key"
    SHARED.interactableDist = dist

    if Input.GetKeyDown(GLFW_KEY_E) then
        print("[KeyInteraction] PICKED UP KEY!")

        --------------------------------------------------------
        -- ADD TO INVENTORY
        --------------------------------------------------------
        local invRef = SHARED and SHARED.player_inventory_ref
        if invRef and invRef.items then
            local items = invRef.items
            local item  = items["key"]

            if item then
                item.qty = item.qty + 1
                print("[Inventory] Updated Key qty =", item.qty)
            else
                items["key"] = { name = "Key", qty = 1 }
                print("[Inventory] Added new Key x1")
            end
        else
            print("[KeyInteraction] WARNING: No inventory ref in SHARED")
        end

        --------------------------------------------------------
        -- SAFE REMOVE (NO DESTROY)
        --------------------------------------------------------
        RemoveKeySafely()

        -- cleanup global interactable
        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
