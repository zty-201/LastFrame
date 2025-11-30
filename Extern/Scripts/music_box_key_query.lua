----------------------------------------------------------------
-- music_box_key_query.lua
----------------------------------------------------------------

local REQUIRED_ITEM_ID  = "music_key"
local PLAYER_ENTITY_ID  = 5
local INTERACT_RANGE    = 80.0
local FACING_THRESHOLD  = 0.5

local DEBUG = true

function on_create(Self)
    print("[MusicBoxKeyQuery] Created for entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
local function IsPlayerInRange()
    local boxTransform = GetEntityTransform(EntityID)

    if not boxTransform then
        print("[MusicBoxKeyQuery] ERROR: Transform NIL for", EntityID)
        return false
    end

    local playerTransform = GetEntityTransform(PLAYER_ENTITY_ID)
    if not playerTransform then return false end

    local px, py, pz = playerTransform:GetPosition()
    local ix, iy, iz = boxTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    return dist <= INTERACT_RANGE, px, py, pz, ix, iy, iz
end

----------------------------------------------------------------
-- Facing check
----------------------------------------------------------------
local function IsPlayerFacing(px, py, pz, ix, iy, iz)
    local cam = GetMainCamera()
    if not cam then return false end

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
        print(string.format("[MusicBoxKeyQuery] FacingDot = %.3f (need %.3f)",
            dot, FACING_THRESHOLD))
    end

    return dot > FACING_THRESHOLD
end

----------------------------------------------------------------
-- Inventory check for MUSIC KEY
----------------------------------------------------------------
local function PlayerHasMusicKey()
    if DEBUG then print("[MusicBoxKeyQuery] Checking inventory...") end

    local inv = SHARED["player_inventory_ref"]
    if not inv then
        print("[MusicBoxKeyQuery] ERROR: inventory ref NIL")
        return false
    end

    local item = inv.items[REQUIRED_ITEM_ID]

    if DEBUG then
        if item then
            print("[MusicBoxKeyQuery] MUSIC KEY FOUND qty =", item.qty)
        else
            print("[MusicBoxKeyQuery] NO MUSIC KEY FOUND")
        end
    end

    return item and item.qty > 0
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    EntityTransform:GetPosition() -- keep your line

    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end

    if not IsPlayerFacing(px, py, pz, ix, iy, iz) then return end

    -- SHOW INTERACTABLE UI
    if PlayerHasMusicKey() then
        SHARED.musicbox_icon_status = "unlock"
        print("[MusicBoxKeyQuery] >>> UNLOCK <<<")
    else
        SHARED.musicbox_icon_status = "lock"
        print("[MusicBoxKeyQuery] >>> LOCK <<<")
        return
    end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "music_box"
    SHARED.interactableDist = 0

    ----------------------------------------------------------------
    -- WHEN E IS PRESSED → CONSUME THE MUSIC KEY
    ----------------------------------------------------------------
    if Input.GetKeyDown(GLFW_KEY_E) and PlayerHasMusicKey() then
        print("[MusicBoxKeyQuery] E pressed → using music key!")

        local inv = SHARED.player_inventory_ref
        local item = inv.items[REQUIRED_ITEM_ID]

        if item then
            item.qty = item.qty - 1
            print("[MusicBoxKeyQuery] Music key consumed. Remaining:", item.qty)

            if item.qty <= 0 then
                inv.items[REQUIRED_ITEM_ID] = nil
                print("[MusicBoxKeyQuery] Music key removed from inventory.")
            end
        end

        -- (OPTIONAL): trigger animation, open box, reveal object...  
        -- Tell me what you want this to do.
    end
end
