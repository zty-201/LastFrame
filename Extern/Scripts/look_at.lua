----------------------------------------------------------------
-- look_at.lua
-- Attach to PLAYER
----------------------------------------------------------------

local LOOK_RANGE = 500.0
local lastHit = nil

function on_create(Self)
    print("[LookAt] Ready")
end

----------------------------------------------------------------
-- Raycast forward from the actual camera
----------------------------------------------------------------
local function RaycastFromCamera()
    local cam = GetMainCamera()      -- FIX: use real camera
    if not cam then 
        print("[LookAt] ERROR: No main camera found!")
        return nil 
    end

    local eye = cam:GetPosition()    -- FIX: correct camera API
    local dir = cam:GetForward()     -- FIX: correct camera API

    local hit = Physics.Raycast(eye, dir, LOOK_RANGE)
    return hit
end

----------------------------------------------------------------
function on_update(Self, dt)

    local hit = RaycastFromCamera()
    lastHit = hit

    if hit and hit.entity then
        print("[LookAt] Looking at entity:", hit.entity, 
              " distance=", hit.distance)

        -- FIX #2: Correct SHARED key name
        SHARED["look_target"] = hit.entity
    else
        SHARED["look_target"] = nil
    end
end
