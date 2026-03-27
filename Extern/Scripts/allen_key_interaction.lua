----------------------------------------------------------------
-- allenkey_interaction.lua (SAFE NO-DESTROY VERSION + UI 107)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 70.0
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

    -- 2. Move far away
    local ix, iy, iz = EntityTransform:GetPosition()
    EntityTransform:SetPosition(ix, iy - 99999, iz)

    -- 3. Disable script
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
    local facing        = IsFacing()

    ------------------------------------------------------------
    -- UI 107 visibility logic
    ------------------------------------------------------------
    local ui107 = GetEntityUI(107)
    if ui107 then
        if inRange and facing then
            ui107:SetVisible(true)
        else
            ui107:SetVisible(false)
        end
    end

    -- Block pickup if invalid
    if not inRange then return end
    if not facing then return end

    SHARED.interactable     = EntityID
    SHARED.interactableType = "allen_key"
    SHARED.interactableDist = dist

    ------------------------------------------------------------
    -- INTERACTION
    ------------------------------------------------------------
    -- local SH     = rawget(_G, "SHARED")
    -- local uiReq  = SH and SH.UI and SH.UI.actionRequested
    -- if uiReq then
    --     SH.UI.actionRequested = false
    -- end


    local SH = rawget(_G, "SHARED")
    local uiReq = SH and SH.UI and SH.UI.actionRequested

    if Input.GetKeyDown(GLFW_KEY_E) or uiReq then
        print("[AllenKey] PICKED UP ALLEN KEY!")

        if SH and SH.UI then
            SH.UI.actionRequested = false
        end

        --------------------------------------------------------
        -- INVENTORY LOGIC
        --------------------------------------------------------
        local invRef = SHARED and SHARED.player_inventory_ref
        if invRef and invRef.items then

            local items     = invRef.items
            local metalPick = items["metal_pick"]
            local allenKey  = items["allen_key"]
            local lockPick  = items["lock_pick"]

            -- CASE 1 — convert metal_pick → lock_pick
            if metalPick and metalPick.qty > 0 then
                print("[AllenKey] Metal Pick detected → Crafting Lock Pick")

                ----------------------------------------------------------------
                -- AUDIO ADDED HERE (plays when crafting happens)
                ----------------------------------------------------------------
                if EntityAudio then
                    EntityAudio:SetLoop(false)
                    EntityAudio:RequestPlay()
                    print("[AllenKey] Played crafting sound.")
                else
                    print("[AllenKey] WARNING: No AudioComponent on entity!")
                end
                ----------------------------------------------------------------

                metalPick.qty = metalPick.qty - 1
                if metalPick.qty <= 0 then
                    items["metal_pick"] = nil
                end

                if lockPick then
                    lockPick.qty = lockPick.qty + 1
                else
                    items["lock_pick"] = { name = "Lock Pick", qty = 1 }
                end

                print("[Inventory] metal_pick → lock_pick SUCCESS")

            -- CASE 2 — pickup Allen Key normally
            else
                print("[AllenKey] No Metal Pick → adding normal Allen Key")

                ----------------------------------------------------------------
                -- AUDIO ADDED HERE (plays when allen key picked normally)
                ----------------------------------------------------------------
                if EntityAudio then
                    EntityAudio:SetLoop(false)
                    EntityAudio:RequestPlay()
                    print("[AllenKey] Played pickup sound.")
                else
                    print("[AllenKey] WARNING: No AudioComponent on entity!")
                end
                ----------------------------------------------------------------

                if allenKey then
                    allenKey.qty = allenKey.qty + 1
                else
                    items["allen_key"] = { name = "Allen Key", qty = 1 }
                end
            end

        else
            print("[AllenKey] ERROR: No inventory reference!")
        end

        -- Hide UI 107 on pickup
        if ui107 then ui107:SetVisible(false) end

        -- Remove Allen Key safely
        SafeRemoveAllenKey()

        SHARED.interactable     = nil
        SHARED.interactableType = nil
        SHARED.interactableDist = nil
    end
end
