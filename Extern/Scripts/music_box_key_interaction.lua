----------------------------------------------------------------
-- musicbox_key_interaction.lua (SAFE NO-DESTROY VERSION + UI 106)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 70.0
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
local function SafeRemoveKey()
    print("[MusicKey] Safely removing key WITHOUT DestroyEntity")

    EntityTransform:SetScale(0, 0, 0)

    local ix, iy, iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

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
    local facing        = IsFacing()

    ------------------------------------------------------------
    -- UI 106 visibility logic
    ------------------------------------------------------------
    local ui106 = GetEntityUI(106)
    if ui106 then
        if inRange and facing then
            ui106:SetVisible(true)
        else
            ui106:SetVisible(false)
        end
    end

    if not inRange then return end
    if not facing then return end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "music_key"
    SHARED.interactableDist = dist

    ------------------------------------------------------------
    -- PICKUP
    ------------------------------------------------------------
    local SH     = rawget(_G, "SHARED")
    local uiReq  = SH and SH.UI and SH.UI.actionRequested
    if uiReq then
        SH.UI.actionRequested = false
    end

    if Input.GetKeyDown(GLFW_KEY_E) or uiReq then
        print("[MusicKey] PICKED UP MUSIC BOX KEY!")

        --------------------------------------------------------
        -- AUDIO PLAY (ONLY ADDITIONS, NO EXISTING CODE MODIFIED)
        --------------------------------------------------------
        if EntityAudio then
            EntityAudio:SetLoop(false)
            EntityAudio:RequestPlay()
            print("[MusicKey] Played pickup sound via EntityAudio.")
        else
            print("[MusicKey] WARNING: No AudioComponent on this key entity!")
        end

        if Play3D_ECS then
            Play3D_ECS(EntityID)
            print("[MusicKey] Played pickup sound via Play3D_ECS.")
        end
        --------------------------------------------------------

        -- Add to inventory
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

        if ui106 then ui106:SetVisible(false) end

        SafeRemoveKey()

        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
