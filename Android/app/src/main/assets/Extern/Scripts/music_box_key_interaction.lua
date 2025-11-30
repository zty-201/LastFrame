----------------------------------------------------------------
-- musicbox_key_interaction.lua (SAFE NO-DESTROY VERSION)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 120.0
local FACING_THRESHOLD = 0.5

function on_create(self)
    print("[MusicKey] Ready on entity:", EntityID)
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
    local dx,dy,dz = ix - cx, iy - cy, iz - cz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    return (fx*dx + fy*dy + fz*dz) > FACING_THRESHOLD
end

----------------------------------------------------------------
-- SAFE REMOVE FUNCTION
----------------------------------------------------------------
local function SafeRemoveKey()
    print("[MusicKey] Safely removing key WITHOUT DestroyEntity")

    -- 1. Make invisible
    EntityTransform:SetScale(0, 0, 0)

    -- 2. Move far away (removes hitbox)
    local ix, iy, iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

    -- 3. Disable script permanently
    on_update = nil
    on_create = nil
    on_destroy = nil

    print("[MusicKey] Disabled script. Entity removed safely.")
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(self, dt)
    local inRange, dist = IsPlayerInRange()
    if not inRange then return end
    if not IsFacing() then return end

    -- Mark as interactable for UI
    SHARED.interactable     = EntityID
    SHARED.interactableType = "music_key"
    SHARED.interactableDist = dist

    ------------------------------------------------------------
    -- PICKUP
    ------------------------------------------------------------
    if Input.GetKeyDown(GLFW_KEY_E) then
        print("[MusicKey] PICKED UP MUSIC BOX KEY!")

        --------------------------------------------------------
        -- ADD ITEM INTO INVENTORY
        --------------------------------------------------------
        local invRef = SHARED and SHARED.player_inventory_ref
        if invRef and invRef.items then
            local items = invRef.items
            local item  = items["music_key"]

            if item then
                item.qty = item.qty + 1
                print("[Inventory] Updated Music Key qty =", item.qty)
            else
                items["music_key"] = { name = "Music Box Key", qty = 1 }
                print("[Inventory] Added Music Box Key x1")
            end
        else
            print("[MusicKey] ERROR: No inventory ref!")
        end

        --------------------------------------------------------
        -- SAFE REMOVE (NO DESTROY)
        --------------------------------------------------------
        SafeRemoveKey()

        -- Clear SHARED interactable info
        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
