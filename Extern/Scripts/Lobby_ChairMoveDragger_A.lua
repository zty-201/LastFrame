-- Lobby_ChairDragMover_A.lua
-- Shorter chair drag with friction-like slowdown and slight wobble/rotation

local DEBUG = true

local DRAG_DISTANCE = 66.0
local BASE_DRAG_SPEED = 65.0

local DRAG_DIR_X = 0.0
local DRAG_DIR_Z = 1.0

local CHAIR_DRAG_SFX_ID = "ChairDrag"

-- motion tuning
local START_SPEED_MULT = 1.35      -- starts faster
local END_SPEED_MULT   = 0.35      -- slows near the end
local SPEED_JITTER     = 0.18      -- random speed variation each frame

local SIDE_WOBBLE_AMT  = 0.06      -- side drift strength
local SIDE_WOBBLE_FREQ = 14.0      -- wobble speed

local ROTATE_MAX_DEG   = 8.0       -- max yaw offset while dragging
local ROTATE_FREQ      = 10.0      -- rotational wobble speed

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
    dragTime = 0.0

    dirX, dirZ = normalize_2d(DRAG_DIR_X, DRAG_DIR_Z)

    -- perpendicular vector for slight sideways wobble
    perpX = -dirZ
    perpZ = dirX

    local tr = Query.GetTransform(EntityID)
    if tr ~= nil then
        baseRotX, baseRotY, baseRotZ = tr:GetRotation()
    end

    local seedBase = tonumber(EntityID) or 0
    if os ~= nil and type(os.time) == "function" then
        math.randomseed(os.time() + seedBase)
    else
        math.randomseed(11111 + seedBase)
    end

    if DEBUG then
        print(string.format(
            "[Lobby_ChairDragMover_A] Ready | ChairID=%d | Distance=%.2f",
            EntityID, DRAG_DISTANCE
        ))
    end
end

function on_update(self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    SHARED = SHARED or {}
    SHARED.CHAIR_DRAG_CMD = SHARED.CHAIR_DRAG_CMD or {}

    local cmdObj = SHARED.CHAIR_DRAG_CMD[EntityID]
    if cmdObj then
        SHARED.CHAIR_DRAG_CMD[EntityID] = nil

        if cmdObj.cmd == "drag" and not isDragging then
            isDragging = true
            draggedDistance = 0.0
            dragTime = 0.0

            baseRotX, baseRotY, baseRotZ = tr:GetRotation()

            play_3d_sfx(CHAIR_DRAG_SFX_ID)

            if DEBUG then
                print(string.format("[Lobby_ChairDragMover_A] Chair %d started dragging", EntityID))
            end
        end
    end

    if not isDragging then return end

    dragTime = dragTime + dt

    local progress = clamp(draggedDistance / DRAG_DISTANCE, 0.0, 1.0)

    -- friction-like speed falloff
    local speedMult = lerp(START_SPEED_MULT, END_SPEED_MULT, progress)

    -- small random speed jitter
    local jitter = random_range(1.0 - SPEED_JITTER, 1.0 + SPEED_JITTER)

    local step = BASE_DRAG_SPEED * speedMult * jitter * dt
    local remaining = DRAG_DISTANCE - draggedDistance

    if step > remaining then
        step = remaining
    end

    -- side wobble fades out near the end
    local wobbleFade = 1.0 - progress
    local sideOffset = math.sin(dragTime * SIDE_WOBBLE_FREQ) * SIDE_WOBBLE_AMT * wobbleFade

    local moveX = dirX * step + perpX * sideOffset
    local moveZ = dirZ * step + perpZ * sideOffset

    local x, y, z = tr:GetPosition()
    tr:SetPosition(x + moveX, y, z + moveZ)

    -- slight rotation while dragging, also fading near the end
    local yawOffset = math.sin(dragTime * ROTATE_FREQ) * ROTATE_MAX_DEG * wobbleFade
    tr:SetRotation(baseRotX, baseRotY + yawOffset, baseRotZ)

    draggedDistance = draggedDistance + step

    if draggedDistance >= DRAG_DISTANCE - 0.001 then
        isDragging = false

        -- settle back cleanly at the end
        tr:SetRotation(baseRotX, baseRotY, baseRotZ)

        if DEBUG then
            print(string.format("[Lobby_ChairDragMover_A] Chair %d finished dragging", EntityID))
        end
    end
end