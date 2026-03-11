-- Lobby_ChairDragMover_B.lua
-- Longer chair drag with start delay, friction-like slowdown, wobble, and rotation

local DEBUG = true

local START_DELAY = 2.7
local delayTimer = 0.0
local waitingForDelay = false

local DRAG_DISTANCE = 44.0
local BASE_DRAG_SPEED = 40.0

local DRAG_DIR_X = 0.0
local DRAG_DIR_Z = 1.0

local CHAIR_DRAG_SFX_ID = "ChairDrag"

-- motion tuning
local START_SPEED_MULT = 1.30
local END_SPEED_MULT   = 0.40
local SPEED_JITTER     = 0.16

local SIDE_WOBBLE_AMT  = 0.05
local SIDE_WOBBLE_FREQ = 12.0

local ROTATE_MAX_DEG   = 6.0
local ROTATE_FREQ      = 9.0

local isDragging = false
local draggedDistance = 0.0
local dirX = 0.0
local dirZ = 0.0

local perpX = 0.0
local perpZ = 0.0

local dragTime = 0.0
local baseRotX = 0.0
local baseRotY = 0.0
local baseRotZ = 0.0

local function normalize_2d(x, z)
    local len = math.sqrt(x * x + z * z)
    if len <= 0.0001 then
        return 0.0, 0.0
    end
    return x / len, z / len
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function lerp(a, b, t)
    return a + (b - a) * t
end

local function random_range(minVal, maxVal)
    return minVal + (maxVal - minVal) * math.random()
end

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    isDragging = false
    draggedDistance = 0.0
    waitingForDelay = false
    delayTimer = 0.0
    dragTime = 0.0

    dirX, dirZ = normalize_2d(DRAG_DIR_X, DRAG_DIR_Z)

    -- perpendicular vector for sideways wobble
    perpX = -dirZ
    perpZ = dirX

    local tr = Query.GetTransform(EntityID)
    if tr ~= nil then
        baseRotX, baseRotY, baseRotZ = tr:GetRotation()
    end

    math.randomseed(os.time() + EntityID)

    if DEBUG then
        print(string.format(
            "[Lobby_ChairDragMover_B] Ready | ChairID=%d | Distance=%.2f",
            EntityID, DRAG_DISTANCE
        ))
    end
end

function on_update(self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    if SHARED and SHARED.CHAIR_DRAG_CMD and SHARED.CHAIR_DRAG_CMD[EntityID] then
        local cmdObj = SHARED.CHAIR_DRAG_CMD[EntityID]
        SHARED.CHAIR_DRAG_CMD[EntityID] = nil

        if cmdObj.cmd == "drag" and not isDragging and not waitingForDelay then
            waitingForDelay = true
            delayTimer = 0.0
            draggedDistance = 0.0
            dragTime = 0.0

            baseRotX, baseRotY, baseRotZ = tr:GetRotation()

            if DEBUG then
                print("[ChairB] Drag command received, waiting before moving")
            end
        end
    end

    -- Delay before movement
    if waitingForDelay then
        delayTimer = delayTimer + dt

        if delayTimer >= START_DELAY then
            waitingForDelay = false
            isDragging = true
            play_3d_sfx(CHAIR_DRAG_SFX_ID)

            if DEBUG then
                print("[ChairB] Delay finished, chair begins moving")
            end
        end

        return
    end

    if not isDragging then return end

    dragTime = dragTime + dt

    local progress = clamp(draggedDistance / DRAG_DISTANCE, 0.0, 1.0)

    -- friction-like slowdown
    local speedMult = lerp(START_SPEED_MULT, END_SPEED_MULT, progress)

    -- small speed variation
    local jitter = random_range(1.0 - SPEED_JITTER, 1.0 + SPEED_JITTER)

    local step = BASE_DRAG_SPEED * speedMult * jitter * dt
    local remaining = DRAG_DISTANCE - draggedDistance

    if step > remaining then
        step = remaining
    end

    -- sideways wobble fades near the end
    local wobbleFade = 1.0 - progress
    local sideOffset = math.sin(dragTime * SIDE_WOBBLE_FREQ) * SIDE_WOBBLE_AMT * wobbleFade

    local moveX = dirX * step + perpX * sideOffset
    local moveZ = dirZ * step + perpZ * sideOffset

    local x, y, z = tr:GetPosition()
    tr:SetPosition(x + moveX, y, z + moveZ)

    -- slight rotational wobble while dragging
    local yawOffset = math.sin(dragTime * ROTATE_FREQ) * ROTATE_MAX_DEG * wobbleFade
    tr:SetRotation(baseRotX, baseRotY + yawOffset, baseRotZ)

    draggedDistance = draggedDistance + step

    if draggedDistance >= DRAG_DISTANCE - 0.001 then
        isDragging = false

        -- settle cleanly at the end
        tr:SetRotation(baseRotX, baseRotY, baseRotZ)

        if DEBUG then
            print(string.format("[Lobby_ChairDragMover_B] Chair %d finished dragging", EntityID))
        end
    end
end