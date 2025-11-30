----------------------------------------------------------------
-- music_box_key_query.lua (with UI 109 + 110)
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
    if not boxTransform then return false end

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

    return (fx*dx + fy*dy + fz*dz) > FACING_THRESHOLD
end

----------------------------------------------------------------
-- Inventory check
----------------------------------------------------------------
local function PlayerHasMusicKey()
    local inv = SHARED["player_inventory_ref"]
    if not inv then return false end

    local item = inv.items[REQUIRED_ITEM_ID]
    return item and item.qty > 0
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    -- ALWAYS HIDE UI (109 locked, 110 unlocked)
    local uiLocked  = GetEntityUI(109)
    local uiUnlock  = GetEntityUI(110)

    if uiLocked then uiLocked:SetVisible(false) end
    if uiUnlock then uiUnlock:SetVisible(false) end

    -------------------------------------------------------
    -- Check distance + facing
    -------------------------------------------------------
    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end

    if not IsPlayerFacing(px, py, pz, ix, iy, iz) then return end

    -------------------------------------------------------
    -- Show UI states
    -------------------------------------------------------
    if PlayerHasMusicKey() then
        -- Player can unlock → show UI 110
        if uiUnlock then uiUnlock:SetVisible(true) end
        SHARED.musicbox_icon_status = "unlock"
        print("[MusicBoxKeyQuery] >>> UNLOCK <<<")
    else
        -- Player missing key → show UI 109
        if uiLocked then uiLocked:SetVisible(true) end
        SHARED.musicbox_icon_status = "lock"
        print("[MusicBoxKeyQuery] >>> LOCK <<<")
        return
    end

    -------------------------------------------------------
    -- Mark as interactable for UI system
    -------------------------------------------------------
    SHARED.interactable     = EntityID
    SHARED.interactableType = "music_box"
    SHARED.interactableDist = 0

    -------------------------------------------------------
    -- Use key (E pressed)
    -------------------------------------------------------
    local SH     = rawget(_G, "SHARED")
    local uiReq  = SH and SH.UI and SH.UI.actionRequested
    if uiReq then
        SH.UI.actionRequested = false
    end

    if Input.GetKeyDown(GLFW_KEY_E) or uiReq and PlayerHasMusicKey() then
        print("[MusicBoxKeyQuery] E pressed → opening music box!")

        -------------------------------------------------------
        -- AUDIO PLAY HERE (ONLY ADDITION)
        -------------------------------------------------------
        if EntityAudio then
            EntityAudio:SetLoop(false)
            EntityAudio:RequestPlay()
            print("[MusicBoxKeyQuery] Played music box unlock sound.")
        else
            print("[MusicBoxKeyQuery] WARNING: No AudioComponent found!")
        end
        -------------------------------------------------------

        local inv = SHARED.player_inventory_ref
        local item = inv.items[REQUIRED_ITEM_ID]

        if item then
            item.qty = item.qty - 1
            print("[MusicBoxKeyQuery] Music key consumed. Remaining:", item.qty)

            local ui19 = GetEntityUI(109)
                ui19:SetSize(0, 0)
            local ui10 = GetEntityUI(110)
                ui10:SetSize(0, 0)

            if item.qty <= 0 then
                inv.items[REQUIRED_ITEM_ID] = nil
                print("[MusicBoxKeyQuery] Removed music_key from inventory.")
            end
        end

        -- TODO: Place what happens when the music box opens
    end
end
