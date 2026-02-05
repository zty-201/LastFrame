-- Door_SharedControlled.lua
-- Attach to Door entity
-- Opens ONCE, then permanently ignores all interaction

local closedAngle = 0.0
local openAngle   = 80.0
local rotateSpeed = 180.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

-- ✅ NEW: one-time open flag
local hasOpenedOnce = false

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

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    print(string.format("[Door] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    ----------------------------------------------------------------
    -- Consume interaction command (ONE TIME ONLY)
    ----------------------------------------------------------------
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil   -- consume it

        -- 🚫 Already opened once → ignore everything forever
        if hasOpenedOnce then
            print("[Door] Ignored interaction (already opened once)")
            return
        end

        -- Allow only first open / toggle
        if cmdObj.cmd == "open" or cmdObj.cmd == "toggle" then
            isOpen = true
            hasOpenedOnce = true
            targetY = openAngle

            print(string.format(
                "[Door] OPENED ONCE by=%s -> permanently disabled",
                tostring(cmdObj.by)
            ))
        else
            -- close command before opening is ignored
            print("[Door] Ignored cmd before first open:", tostring(cmdObj.cmd))
        end
    end

    ----------------------------------------------------------------
    -- Rotation logic (UNCHANGED)
    ----------------------------------------------------------------
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
