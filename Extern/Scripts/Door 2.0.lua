-- Door_InRange_Working.lua
-- Attach to Door entity
-- Opens/closes with E when player is in range

local closedAngle = 0.0
local openAngle   = 80.0
local rotateSpeed = 180.0
local interactRange = 250.0   -- BIGGER range, as requested

local PLAYER_ID = 5         -- confirmed by you

local isOpen = false
local currentY = 0.0
local targetY = 0.0

function on_create(Self)
    print("[Door] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[Door] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    print(string.format("[Door] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    -- ONLY difference: add in-range gate
    if Input.GetKeyDown(GLFW_KEY_E) then
        local inRange = Query.IsEntityInRange(EntityID, PLAYER_ID, interactRange)

        print(string.format(
            "[Door] E pressed. inRange=%s (range=%.1f)",
            tostring(inRange),
            interactRange
        ))

        if inRange then
            isOpen = not isOpen
            targetY = isOpen and openAngle or closedAngle
            print(string.format(
                "[Door] TOGGLE -> isOpen=%s targetY=%.2f",
                tostring(isOpen),
                targetY
            ))
        end
    end

    -- Rotation logic UNCHANGED
    local diff = targetY - currentY
    if math.abs(diff) > 0.05 then
        local dir = (diff > 0) and 1 or -1
        currentY = currentY + dir * rotateSpeed * dt

        if dir > 0 then
            if currentY > targetY then currentY = targetY end
        else
            if currentY < targetY then currentY = targetY end
        end

        local rx, _, rz = tr:GetRotation()
        tr:SetRotation(rx, currentY, rz)
    end
end
