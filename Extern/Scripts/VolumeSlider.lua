----------------------------------------------------------------
-- VolumeSlider.lua
--
-- Attach to the HANDLE entity (SliderControllerVolume / SliderController).
-- The entity must have a UIComponent with is_NDC = true.
--
-- The renderer overwrites UIComponent.position every frame from
-- position_NDC, so we drive the visual by writing position_NDC.x.
-- position / size (pixels) are then always up-to-date for hit testing.
--
-- Track NDC constants come from the SliderBar / SliderBarVolume entity:
--   position_NDC.x = 0.13  (center X of track)
--   size_NDC.x     = 0.45  (width of track)
--
-- Configurable:
--   AUDIO_GROUP  — which AudioGroup to control
--   SHARED_KEY   — key in SHARED table for cross-script volume sharing
----------------------------------------------------------------

local AUDIO_GROUP = AudioGroup and AudioGroup.Master or 0
local SHARED_KEY  = "masterVol"

-- Track NDC values (from SliderBarVolume json)
-- position_NDC is the CENTER, so track left edge NDC = center - half_width
local TRACK_NDC_CX = 0.13   -- position_NDC.x of the track entity
local TRACK_NDC_W  = 0.45   -- size_NDC.x of the track entity
local HANDLE_NDC_W = 0.02   -- size_NDC.x of this handle entity

-- ── local state ────────────────────────────────────────────────
local dragging    = false
local handleNdcY  = 0.0   -- cached on create, stays constant

-- ── helpers ────────────────────────────────────────────────────
local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- Returns the track's left edge and total width in pixels at current window size.
local function trackPixelBounds()
    local W, _ = Input.GetWindowSize()
    -- center pixel of track = (NDC + 1) * 0.5 * W
    -- left edge = center - half_width
    local trackCenterPx = (TRACK_NDC_CX + 1.0) * 0.5 * W
    local trackWidthPx  = TRACK_NDC_W * W          -- size_NDC * screen
    local trackLeftPx   = trackCenterPx - trackWidthPx * 0.5
    local handleWidthPx = HANDLE_NDC_W * W
    return trackLeftPx, trackWidthPx, handleWidthPx
end

-- Is the UI mouse cursor inside the handle's current pixel bounds?
local function isOverHandle()
    if not EntityUI then return false end
    local mx, my = Input.GetUIMousePosition()
    local px, py = EntityUI:GetPosition()
    local pw, ph = EntityUI:GetSize()
    return mx >= px and mx <= px + pw and my >= py and my <= py + ph
end

-- Compute normalised value [0,1] from mouse X relative to the track.
local function computeValue(mx)
    local trackLeft, trackWidth, _ = trackPixelBounds()
    return clamp((mx - trackLeft) / trackWidth, 0.0, 1.0)
end

-- Move the handle to reflect a given value and return the NDC X used.
local function moveHandle(value)
    if not EntityUI then return end
    local W, _ = Input.GetWindowSize()
    local trackLeft, trackWidth, handleWidth = trackPixelBounds()

    -- Handle left-edge pixel = track left + value * (track width - handle width)
    local handleLeftPx   = trackLeft + value * (trackWidth - handleWidth)
    -- position_NDC is the CENTER of the element
    local handleCenterPx = handleLeftPx + handleWidth * 0.5
    -- NDC = center_px / (0.5 * W) - 1
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

    -- Cache the handle's Y NDC so we only ever change X.
    if EntityUI then
        local _, ndcY = EntityUI:GetPosition_NDC()
        handleNdcY = ndcY
    end

    -- Snap handle to reflect the saved volume
    moveHandle(SHARED[SHARED_KEY])
    Audio_SetMasterVolume(SHARED[SHARED_KEY])

    print(string.format("[VolumeSlider] on_create  vol=%.2f", SHARED[SHARED_KEY]))
end

function on_update(self, dt)
    local mx, my = Input.GetUIMousePosition()

    local down     = Input.GetMouseButtonDown(0)
    local held     = Input.GetMouseButton(0)
    local released = Input.GetMouseButtonUp(0)

    -- Start drag: press must originate on the handle
    if down and isOverHandle() then
        dragging = true
    end

    -- Update value while drag is active
    if dragging and held then
        local vol = computeValue(mx)
        moveHandle(vol)
        applyVolume(vol)
    end

    -- End drag
    if released then
        dragging = false
    end
end

function on_destroy(self)
    dragging = false
    print("[VolumeSlider] on_destroy")
end
