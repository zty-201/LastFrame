----------------------------------------------------------------
-- door_query_debug.lua
----------------------------------------------------------------

local REQUIRED_ITEM_ID  = "lock_pick"
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
-- Facing check (debug)
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
    if DEBUG then print("[DoorQuery] Checking inventory...") end

    local inv = SHARED["player_inventory_ref"]
    if not inv then 
        -- print("[DoorQuery] ERROR: player_inventory_ref is NIL")
        return false 
    end

    local item = inv.items[REQUIRED_ITEM_ID]

    if DEBUG then
        if item then
            -- print("[DoorQuery] KEY FOUND qty =", item.qty)
        else
            -- print("[DoorQuery] NO KEY FOUND")
        end
    end

    return item and item.qty > 0
end


----------------------------------------------------------------
-- on_update (with debug)
----------------------------------------------------------------
function on_update(Self, dt)

    EntityTransform:GetPosition()

    -- ALWAYS HIDE BOTH UI ICONS FIRST
    local ui103 = GetEntityUI(111)
    local ui104 = GetEntityUI(112)
    if ui103 then ui103:SetVisible(false) end
    if ui104 then ui104:SetVisible(false) end

    local inRange, px, py, pz, ix, iy, iz = IsPlayerInRange()
    if not inRange then return end

    if not IsPlayerFacingDoor(px, py, pz, ix, iy, iz) then return end



    -- ICON & INTERACTION INDICATOR
    if PlayerHasKey() then

        -- SHOW 103 (unlock)
        local ui103 = GetEntityUI(112)
        if ui103 then ui103:SetVisible(true) end

        -- HIDE 104 (locked)
        local ui104 = GetEntityUI(111)
        if ui104 then ui104:SetVisible(false) end

        SHARED.door_icon_status = "unlock"

    else

        -- SHOW 104 (locked)
        local ui104 = GetEntityUI(111)
        if ui104 then ui104:SetVisible(true) end

        -- HIDE 103 (unlock)
        local ui103 = GetEntityUI(112)
        if ui103 then ui103:SetVisible(false) end

        SHARED.door_icon_status = "lock"
        return
    end


    SHARED.interactable     = EntityID
    SHARED.interactableType = "door"
    SHARED.interactableDist = 0


    ----------------------------------------------------------------
    -- E PRESSED → USE KEY → OPEN DOOR
    ----------------------------------------------------------------
    if Input.GetKeyDown(GLFW_KEY_E) and PlayerHasKey() then
        print("[DoorQuery] E pressed → opening door")

        --------------------------------------------------------
        -- (1) Consume the key
        --------------------------------------------------------
        local inv = SHARED.player_inventory_ref
        local keyItem = inv.items["lock_pick"]

        if keyItem then
            keyItem.qty = keyItem.qty - 1
            print("[DoorQuery] Key consumed. Remaining:", keyItem.qty)

            local ui111 = GetEntityUI(111)
                ui111:SetSize(0, 0)
            local ui112 = GetEntityUI(112)
                ui112:SetSize(0, 0)


            if keyItem.qty <= 0 then
                inv.items["lock_pick"] = nil
                print("[DoorQuery] Key removed from inventory.")
            end
        end

        --------------------------------------------------------
        -- (2) SCALE ENTITY 29 → 0,0,0 (hide original door)
        --------------------------------------------------------
        local t29 = GetEntityTransform(46)
        if t29 then
            t29:SetScale(0,0,0)
            print("[DoorQuery] Entity 26 hidden successfully.")
        else
            print("[DoorQuery] ERROR: Transform for entity 46 not found!")
        end

        --------------------------------------------------------
        -- (3) SCALE ENTITY 101 → 30,30,30 (show opened door)
        --------------------------------------------------------
        local t101 = GetEntityTransform(113)
        if t101 then
            t101:SetScale(30,30,30)
            print("[DoorQuery] Entity 113 scaled to 30,30,30 (opened door).")
        else
            print("[DoorQuery] ERROR: Transform for entity 113 not found!")
        end
    end
end
