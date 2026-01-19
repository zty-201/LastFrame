----------------------------------------------------------------
-- teddy_with_note_interaction.lua (DEBUG + NO DESTROY)
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5
local INTERACT_RANGE   = 120.0
local FACING_THRESHOLD = 0.5

-- TODO: Replace this with your actual PNG UI entity ID
local BEAR_UI_ENTITY_ID = 666   -- <---- CHANGE THIS

function on_create(self)
    print("[TeddyNote] Loaded for entity:", EntityID)
end

----------------------------------------------------------------
-- Distance check
----------------------------------------------------------------
local function IsPlayerInRange()
    local pt = GetEntityTransform(PLAYER_ENTITY_ID)
    if not pt then 
        print("[TeddyNote] ERROR: Player transform NIL")
        return false 
    end

    local px,py,pz = pt:GetPosition()
    local ix,iy,iz = EntityTransform:GetPosition()

    local dx = px - ix
    local dy = py - iy
    local dz = pz - iz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    print("[TeddyNote] Dist =", dist)

    return dist <= INTERACT_RANGE, dist
end

----------------------------------------------------------------
-- Facing check
----------------------------------------------------------------
local function IsFacing()
    local cam = GetMainCamera()
    if not cam then 
        print("[TeddyNote] ERROR: Camera missing")
        return false 
    end

    local cx,cy,cz = cam:GetPosition()
    local tx,ty,tz = cam:GetTarget()

    local fx,fy,fz = tx - cx, ty - cy, tz - cz
    local fm = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx,fy,fz = fx/fm, fy/fm, fz/fm

    local ix,iy,iz = EntityTransform:GetPosition()
    local dx,dy,dz = ix - cx, iy - cy, iz - cz
    local dm = math.sqrt(dx*dx + dy*dy + dz*dz)
    dx,dy,dz = dx/dm, dy/dm, dz/dm

    local dot = fx*dx + fy*dy + fz*fz
    print("[TeddyNote] FacingDot =", dot)

    return dot > FACING_THRESHOLD
end

----------------------------------------------------------------
-- Show the Bear PNG (UIComponent)
----------------------------------------------------------------
local function ShowBearPNG()
    print("[TeddyNote] Attempting to show Bear UI...")

    local uiComp = GetEntityUI(BEAR_UI_ENTITY_ID)
    if not uiComp then
        print("[TeddyNote] ERROR: Bear UI entity missing UIComponent!")
        return
    end

    uiComp:SetVisible(true)
    print("[TeddyNote] Bear PNG is now VISIBLE")
end

----------------------------------------------------------------
-- on_update (main logic)
----------------------------------------------------------------
function on_update(self, dt)

    print("--------------- [TeddyNote] FRAME ----------------")

    local inRange, dist = IsPlayerInRange()
    if not inRange then 
        print("[TeddyNote] NOT in range")
        return 
    end

    if not IsFacing() then
        print("[TeddyNote] NOT facing teddy")
        return
    end

    print("[TeddyNote] IN RANGE + FACING!")

    -- Mark as interactable
    SHARED.interactable     = EntityID
    SHARED.interactableType = "teddy_note"
    SHARED.interactableDist = dist

    -- Interaction
    local SH     = rawget(_G, "SHARED")
    local uiReq  = SH and SH.UI and SH.UI.actionRequested
    if uiReq then
        SH.UI.actionRequested = false
    end

    if Input.GetKeyDown(GLFW_KEY_E) or uiReq then
        print("[TeddyNote] E PRESSED Showing bear PNG")
        ShowBearPNG()
    end
end
