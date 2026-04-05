----------------------------------------------------------------
-- GyroSlider.lua
--
-- Attach to the gyro slider HANDLE entity (SliderController, entity 1059).
-- The entity must have a UIComponent with is_NDC = true.
--
-- Writes SHARED.GYRO_SENSITIVITY, which player_pov_camera.lua reads
-- every frame (line 563). Range: 0.0 -> 2.0, default 1.0 at centre.
--
-- Track NDC values come from SliderBar (entity 1060):
--   position_NDC.x = 0.13  (centre X of track)
--   size_NDC.x     = 0.45  (width of track)
----------------------------------------------------------------

local MIN_SENS  = 0.0
local MAX_SENS  = 2.0
local DEF_SENS  = 1.0   -- sits exactly at slider centre (normalised 0.5)

local TRACK_NDC_CX = 0.13
local TRACK_NDC_W  = 0.45
local HANDLE_NDC_W = 0.02

-- ── local state ────────────────────────────────────────────────
local dragging   = false
local handleNdcY = 0.0

-- ── helpers ────────────────────────────────────────────────────
local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function sensToNorm(sens)
    return clamp((sens - MIN_SENS) / (MAX_SENS - MIN_SENS), 0.0, 1.0)
end

local function normToSens(norm)
    return MIN_SENS + norm * (MAX_SENS - MIN_SENS)
end

local function trackPixelBounds()
    local W, _ = Input.GetWindowSize()
    local trackCenterPx = (TRACK_NDC_CX + 1.0) * 0.5 * W
    local trackWidthPx  = TRACK_NDC_W * W
    local trackLeftPx   = trackCenterPx - trackWidthPx * 0.5
    local handleWidthPx = HANDLE_NDC_W * W
    return trackLeftPx, trackWidthPx, handleWidthPx
end

local function isOverHandle()
    if not EntityUI then return false end
    local mx, my = Input.GetUIMousePosition()
    local px, py = EntityUI:GetPosition()
    local pw, ph = EntityUI:GetSize()
    return mx >= px and mx <= px + pw and my >= py and my <= py + ph
end

local function computeNorm(mx)
    local trackLeft, trackWidth, _ = trackPixelBounds()
    return clamp((mx - trackLeft) / trackWidth, 0.0, 1.0)
end

local function moveHandle(norm)
    if not EntityUI then return end
    local W, _ = Input.GetWindowSize()
    local trackLeft, trackWidth, handleWidth = trackPixelBounds()
    local handleLeftPx   = trackLeft + norm * (trackWidth - handleWidth)
    local handleCenterPx = handleLeftPx + handleWidth * 0.5
    local handleNdcX     = handleCenterPx * 2.0 / W - 1.0
    EntityUI:SetPosition_NDC(handleNdcX, handleNdcY)
end

-- ── lifecycle ──────────────────────────────────────────────────
function on_create(self)
    SHARED = SHARED or {}

    if SHARED.GYRO_SENSITIVITY == nil then
        SHARED.GYRO_SENSITIVITY = DEF_SENS
    end

    if EntityUI then
        local _, ndcY = EntityUI:GetPosition_NDC()
        handleNdcY = ndcY
    end

    -- Snap handle to current (or default) sensitivity
    moveHandle(sensToNorm(SHARED.GYRO_SENSITIVITY))

    print(string.format("[GyroSlider] on_create  sens=%.2f", SHARED.GYRO_SENSITIVITY))
end

function on_update(self, dt)
    local mx, my = Input.GetUIMousePosition()

    local down     = Input.GetMouseButtonDown(0)
    local held     = Input.GetMouseButton(0)
    local released = Input.GetMouseButtonUp(0)

    if down and isOverHandle() then
        dragging = true
    end

    if dragging and held then
        local norm = computeNorm(mx)
        moveHandle(norm)
        SHARED = SHARED or {}
        SHARED.GYRO_SENSITIVITY = normToSens(norm)
    end

    if released then
        dragging = false
    end
end

function on_destroy(self)
    dragging = false
    print("[GyroSlider] on_destroy")
end
