----------------------------------------------------------------
-- VolumeSlider_Pause.lua
--
-- Attach to the volume slider HANDLE entity in PauseMenu.
-- The entity must have a UIComponent with is_NDC = true.
--
-- Track NDC values come from the PauseMenu SliderBarVolume entity:
--   position_NDC.x = 0.26  (centre X of track)
--   size_NDC.x     = 0.53  (width of track)
--   (corresponds to NDC x range -0.270 to 0.79)
----------------------------------------------------------------

local AUDIO_GROUP = AudioGroup and AudioGroup.Master or 0
local SHARED_KEY  = "masterVol"

local TRACK_NDC_CX = 0.26
local TRACK_NDC_W  = 0.53
local HANDLE_NDC_W = 0.02

-- ── local state ────────────────────────────────────────────────
local dragging    = false
local handleNdcY  = 0.0

-- ── helpers ────────────────────────────────────────────────────
local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
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

local function computeValue(mx)
    local trackLeft, trackWidth, _ = trackPixelBounds()
    return clamp((mx - trackLeft) / trackWidth, 0.0, 1.0)
end

local function moveHandle(value)
    if not EntityUI then return end
    local W, _ = Input.GetWindowSize()
    local trackLeft, trackWidth, handleWidth = trackPixelBounds()
    local handleLeftPx   = trackLeft + value * (trackWidth - handleWidth)
    local handleCenterPx = handleLeftPx + handleWidth * 0.5
    local handleNdcX     = handleCenterPx * 2.0 / W - 1.0
    EntityUI:SetPosition_NDC(handleNdcX, handleNdcY)
end

local function applyVolume(vol)
    SHARED = SHARED or {}
    SHARED[SHARED_KEY] = vol
    Audio_SetMasterVolume(vol)
end

-- ── lifecycle ──────────────────────────────────────────────────
function on_create(self)
    SHARED = SHARED or {}
    if SHARED[SHARED_KEY] == nil then
        SHARED[SHARED_KEY] = 1.0
    end

    if EntityUI then
        local _, ndcY = EntityUI:GetPosition_NDC()
        handleNdcY = ndcY
    end

    moveHandle(SHARED[SHARED_KEY])
    Audio_SetMasterVolume(SHARED[SHARED_KEY])

    print(string.format("[VolumeSlider_Pause] on_create  vol=%.2f", SHARED[SHARED_KEY]))
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
        local vol = computeValue(mx)
        moveHandle(vol)
        applyVolume(vol)
    end

    if released then
        dragging = false
    end
end

function on_destroy(self)
    dragging = false
    print("[VolumeSlider_Pause] on_destroy")
end
