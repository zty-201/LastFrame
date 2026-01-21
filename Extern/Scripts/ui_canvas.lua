-- ============================================================
-- ui_canvas.lua
-- Central UI sizing + helpers (Windows + Android)
-- Requires: Input.GetWindowSize()
-- ============================================================

local DESIGN_HEIGHT =  1440.0  -- your "virtual UI height" (pick one and keep it stable)

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local Shared = getShared()
    if not Shared then
        Shared = {}
        _G.SHARED = Shared
    end
    Shared.UI = Shared.UI or {}
    return Shared.UI
end

local function updateUISize()
    local ui = ensureUIShared()

    local Shared = getShared()
    if Shared then
        -- Centralize platform in one place for all scripts
        ui.platform = Shared.PLATFORM or PLATFORM
    end

    local realW, realH = Input.GetWindowSize()
    if not realW or not realH or realW <= 0 or realH <= 0 then
        return
    end

    local aspect = realW / realH

    -- Real window/framebuffer pixels (engine)
    ui.realW = realW
    ui.realH = realH

    -- Virtual UI coordinates (Lua/UI scripts)
    ui.screenH = DESIGN_HEIGHT
    ui.screenW = DESIGN_HEIGHT * aspect

    -- Scale between real pixels and virtual UI units
    ui.scaleX = ui.screenW / ui.realW
    ui.scaleY = ui.screenH / ui.realH

    -- Optional safe padding (keep 0 for now)
    ui.safeLeft, ui.safeRight, ui.safeTop, ui.safeBottom = 0, 0, 0, 0

    -- Helpers for all scripts
    ui.AnchorCenter = function(ax, ay)
        return ax * ui.screenW, ay * ui.screenH
    end

    ui.AnchorRect = function(ax, ay, w, h)
        local cx = ax * ui.screenW
        local cy = ay * ui.screenH
        return (cx - w * 0.5), (cy - h * 0.5)
    end

    -- Convert raw input (real pixels) into UI space (virtual units)
    ui.ToUIX = function(rawMx)
        return rawMx * ui.scaleX
    end

    ui.ToUIY = function(rawMy)
        -- rawMy is typically "pixels from top"
        -- Convert to UI units and flip to bottom-left origin
        local y_ui_top = rawMy * ui.scaleY
        return ui.screenH - y_ui_top
    end
end

function on_create(self)
    updateUISize()
end

function on_update(self, dt)
    updateUISize()
end
