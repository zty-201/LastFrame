local LOOK_RANGE = 500.0

function on_create(self)
    print("[LookAt] Ready")
end

local function RaycastFromCamera()
    local pov = SHARED.POV
    if not pov then return nil end

    local cam = pov.cam
    if not cam then return nil end

    -- Camera world position & forward
    local eye = cam:GetPosition()
    local dir = cam:GetForward()

    -- Real physics ray
    local hit = Physics.Raycast(eye, dir, LOOK_RANGE)
    return hit
end

function on_update(self, dt)
    local hit = RaycastFromCamera()

    if hit and hit.entity then
        print("[LookAt] Looking at:", hit.entity, "dist=", hit.distance)
        SHARED.look_target = hit.entity
    else
        SHARED.look_target = nil
    end
end
