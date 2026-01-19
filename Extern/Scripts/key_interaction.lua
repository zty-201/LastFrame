----------------------------------------------------------------
-- key_interaction.lua (SAFE VERSION + UI 105 HIGHLIGHT)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 80.0
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

    EntityTransform:SetScale(0, 0, 0)

    local ix,iy,iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

    -- disable script
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
    local facing = IsFacing()

    ------------------------------------------------------------
    -- UPDATE UI ENTITY 105 VISIBILITY
    ------------------------------------------------------------
    local ui105 = GetEntityUI(105)
    if ui105 then
        if inRange and facing then
            ui105:SetVisible(true)
        else
            ui105:SetVisible(false)
        end
    end

    ------------------------------------------------------------
    -- No interaction unless both conditions true
    ------------------------------------------------------------
    if not inRange then return end
    if not facing then return end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "key"
    SHARED.interactableDist = dist

    local SH     = rawget(_G, "SHARED")
    local uiReq  = SH and SH.UI and SH.UI.actionRequested
    if uiReq then
        SH.UI.actionRequested = false
    end

    if Input.GetKeyDown(GLFW_KEY_E) or uiReq then
        print("[KeyInteraction] PICKED UP KEY!")

        --------------------------------------------------------
        -- PLAY PICKUP AUDIO HERE (one-shot)
        --------------------------------------------------------
        if EntityAudio then
            EntityAudio:SetLoop(false)      -- ensure no looping
            EntityAudio:RequestPlay()       -- plays once
            print("[KeyInteraction] Played pickup sound")
        else
            print("[KeyInteraction] No AudioComponent found!")
        end

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
        -- HIDE UI 105 ON PICKUP
        --------------------------------------------------------
        if ui105 then ui105:SetVisible(false) end

        --------------------------------------------------------
        -- REMOVE KEY FROM WORLD
        --------------------------------------------------------
        RemoveKeySafely()

        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
