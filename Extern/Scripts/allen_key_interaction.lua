----------------------------------------------------------------
-- allenkey_interaction.lua (SAFE NO-DESTROY VERSION)
-- New behavior: if player already has a metal_pick,
-- convert metal_pick → lock_pick instead of adding allen_key.
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 120.0
local FACING_THRESHOLD = 0.5

function on_create(self)
    print("[AllenKey] Ready on entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
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
    local dx = ix - cx
    local dy = iy - cy
    local dz = iz - cz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    return (fx*dx + fy*dy + fz*dz) > FACING_THRESHOLD
end

----------------------------------------------------------------
-- SAFE REMOVE FUNCTION
----------------------------------------------------------------
local function SafeRemoveAllenKey()
    print("[AllenKey] Safely removing key WITHOUT DestroyEntity")

    -- 1. Hide visually
    EntityTransform:SetScale(0, 0, 0)

    -- 2. Move away to ensure no collision or interaction
    local ix, iy, iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

    -- 3. Disable script permanently
    on_update = nil
    on_create = nil
    on_destroy = nil

    print("[AllenKey] Script disabled, item removed safely.")
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(self, dt)

    local inRange, dist = IsPlayerInRange()
    if not inRange then return end
    if not IsFacing() then return end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "allen_key"
    SHARED.interactableDist = dist

    if Input.GetKeyDown(GLFW_KEY_E) then
        print("[AllenKey] INTERACTED!")

        --------------------------------------------------------
        -- INVENTORY LOGIC (NEW BEHAVIOR)
        --------------------------------------------------------
        local invRef = SHARED and SHARED.player_inventory_ref
        if invRef and invRef.items then

            local items = invRef.items
            local metalPick = items["metal_pick"]
            local allenKey  = items["allen_key"]
            local lockPick  = items["lock_pick"]

            ----------------------------------------------------
            -- CASE 1: Player already has a metal pick
            --   → consume metal_pick
            --   → give lock_pick
            --   → DO NOT give allen_key
            ----------------------------------------------------
            if metalPick and metalPick.qty > 0 then
                print("[AllenKey] Metal Pick detected → Crafting Lock Pick")

                -- Remove 1 metal_pick
                metalPick.qty = metalPick.qty - 1
                if metalPick.qty <= 0 then
                    items["metal_pick"] = nil
                end

                -- Add lock_pick
                if lockPick then
                    lockPick.qty = lockPick.qty + 1
                else
                    items["lock_pick"] = { name = "Lock Pick", qty = 1 }
                end

                print("[Inventory] metal_pick → lock_pick SUCCESS")
            
            ----------------------------------------------------
            -- CASE 2: No metal pick → normal allen key pickup
            ----------------------------------------------------
            else
                print("[AllenKey] No Metal Pick → adding normal Allen Key")

                if allenKey then
                    allenKey.qty = allenKey.qty + 1
                else
                    items["allen_key"] = { name = "Allen Key", qty = 1 }
                end
            end

        else
            print("[AllenKey] ERROR: No inventory reference!")
        end

        --------------------------------------------------------
        -- SAFE REMOVE (NO DESTROY)
        --------------------------------------------------------
        SafeRemoveAllenKey()

        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
