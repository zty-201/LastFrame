----------------------------------------------------------------
-- door_query_debug.lua  (with DOOR OPEN AUDIO)
----------------------------------------------------------------

local REQUIRED_ITEM_ID  = "key"
local PLAYER_ENTITY_ID  = 5
local INTERACT_RANGE    = 80.0
local FACING_THRESHOLD  = 0.5

local DEBUG = true

function on_create(Self)
    print("[DoorQuery] Created for door entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check (debug-enabled)
----------------------------------------------------------------
local function IsPlayerInRange()
    local doorTransform = GetEntityTransform(EntityID)

    if not doorTransform then
        print("[DoorQuery] ERROR: DOOR TRANSFORM NIL for", EntityID)
        return false
    end

    local playerTransform = GetEntityTransform(PLAYER_ENTITY_ID)
    if not playerTransform then return false end

    local px, py, pz = playerTransform:GetPosition()
    local ix, iy, iz = doorTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    return dist <= INTERACT_RANGE, px, py, pz, ix, iy, iz
end

----------------------------------------------------------------
-- Facing check
----------------------------------------------------------------
local function IsPlayerFacingDoor(px, py, pz, ix, iy, iz)
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
        print(string.format("[DoorQuery] FacingDot = %.3f (threshold %.3f)", dot, FACING_THRESHOLD))
    end

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
-- UPDATE
----------------------------------------------------------------
function on_update(Self, dt)

    EntityTransform:GetPosition()

    -- ALWAYS HIDE BOTH ICONS
    local ui103 = GetEntityUI(103)
    local ui104 = GetEntityUI(104)
    if ui103 then ui103:SetVisible(false) end
    if ui104 then ui104:SetVisible(false) end

    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end
    if not IsPlayerFacingDoor(px, py, pz, ix, iy, iz) then return end

    -- SHOW ICONS
    if PlayerHasKey() then
        if ui103 then ui103:SetVisible(true) end
        SHARED.door_icon_status = "unlock"
    else
        if ui104 then ui104:SetVisible(true) end
        SHARED.door_icon_status = "lock"
        return
    end


    SHARED.interactable     = EntityID
    SHARED.interactableType = "door"
    SHARED.interactableDist = 0


    ----------------------------------------------------------------
    -- E PRESSED = OPEN DOOR
    ----------------------------------------------------------------
    if Input.GetKeyDown(GLFW_KEY_E) and PlayerHasKey() then
        print("[DoorQuery] E pressed → opening door")

        --------------------------------------------------------
        -- (AUDIO) Play door unlock sound
        --------------------------------------------------------
        if EntityAudio then
            EntityAudio:SetLoop(false)
            EntityAudio:RequestPlay()
            print("[DoorQuery] Played door unlock sound")
        else
            print("[DoorQuery] WARNING: No AudioComponent on door entity!")
        end

        --------------------------------------------------------
        -- Consume key
        --------------------------------------------------------
        local inv = SHARED.player_inventory_ref
        local keyItem = inv.items["key"]

        if keyItem then
            keyItem.qty = keyItem.qty - 1

            if keyItem.qty <= 0 then
                inv.items["key"] = nil
            end
        end

        -- Hide UI icons permanently
        local ui13 = GetEntityUI(103)
        if ui13 then ui13:SetSize(0, 0) end

        local ui14 = GetEntityUI(104)
        if ui14 then ui14:SetSize(0, 0) end

        --------------------------------------------------------
        -- Hide original door (29)
        --------------------------------------------------------
        local t29 = GetEntityTransform(29)
        if t29 then
            t29:SetScale(0,0,0)
        end

        --------------------------------------------------------
        -- Show opened door (101)
        --------------------------------------------------------
        local t101 = GetEntityTransform(101)
        if t101 then
            t101:SetScale(30,30,30)
        end
    end
end
