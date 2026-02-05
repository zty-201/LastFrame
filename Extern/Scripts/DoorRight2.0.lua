-- Door_SharedControlled_Right.lua
-- RIGHT door opens ONCE, then permanently ignores interaction.

local closedAngle = 0.0
local openAngle   = -80.0
local rotateSpeed = 180.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

-- ✅ NEW: one-time open
local hasOpenedOnce = false

function on_create(Self)
    print("[DoorR] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[DoorR] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    print(string.format("[DoorR] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        -- ✅ already opened once -> ignore known commands forever
        -- If already opened once:
        -- allow CLOSE commands (for scripted events), but block toggle/open
        if hasOpenedOnce then
            if cmdObj.cmd == "close" then
                isOpen = false
                targetY = closedAngle
                print("[Door] Scripted CLOSE allowed after one-time open")
            else
                print("[Door] Ignored interaction (already opened once)")
            end
            return
        end


        -- Only first open/toggle is allowed; force open
        if cmdObj.cmd == "open" or cmdObj.cmd == "toggle" then
            isOpen = true
            hasOpenedOnce = true
            targetY = openAngle

            print(string.format(
                "[DoorR] OPENED ONCE by=%s -> permanently disabled",
                tostring(cmdObj.by)
            ))
        else
            print("[DoorR] Ignored cmd before first open:", tostring(cmdObj.cmd))
        end
    end

    -- Rotation logic
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
