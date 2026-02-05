-- ForwardTransform_Test.lua

local LOG_INTERVAL = 0.5
local t = 0

function on_create(Self)
    print("[ForwardTransform_Test] EntityID:", EntityID)
end

function on_update(Self, dt)
    t = t + dt

    -- Print forward + position every 0.5s
    if t >= LOG_INTERVAL then
        t = 0

        local fx, fy, fz = Query.GetForward(EntityID)
        print(string.format("[Forward] (%.3f, %.3f, %.3f)", fx, fy, fz))

        local tr = Query.GetTransform(EntityID)
        if tr ~= nil then
            local px, py, pz = tr:GetPosition()
            local rx, ry, rz = tr:GetRotation()
            print(string.format("[Pos] (%.3f, %.3f, %.3f)  [Rot] (%.3f, %.3f, %.3f)", px, py, pz, rx, ry, rz))
        else
            print("[ForwardTransform_Test] No Transform found for EntityID")
        end
    end

    -- Press Q to scale yourself bigger, E to scale smaller
    if Input.GetKeyDown(GLFW_KEY_Q) then
        local tr = Query.GetTransform(EntityID)
        if tr ~= nil then
            tr:SetScale(2, 2, 2)
            print("[ForwardTransform_Test] SetScale -> (2,2,2)")
        end
    end

    if Input.GetKeyDown(GLFW_KEY_E) then
        local tr = Query.GetTransform(EntityID)
        if tr ~= nil then
            tr:SetScale(1, 1, 1)
            print("[ForwardTransform_Test] SetScale -> (1,1,1)")
        end
    end
end
