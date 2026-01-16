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
    local SH = getShared()
    if not SH then
        SH = {}
        _G.SHARED = SH
    end
    SH.UI = SH.UI or {}
    return SH.UI
end

local function updateUISize()
    local ui = ensureUIShared()

    local realW, realH = Input.GetWindowSize()
    if not realW or not realH or realW <= 0 or realH <= 0 then
        return
    end

    local aspect = realW / realH

    -- Real window pixels (engine)
    ui.realW = realW
    ui.realH = realH

    -- Virtual UI coordinates (Lua/UI scripts)
    ui.screenH = DESIGN_HEIGHT
    ui.screenW = DESIGN_HEIGHT * aspect

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

    -- Convert raw mouse Y (top-left origin) into UI Y (bottom-left origin)
    ui.ToUIY = function(rawMy)
        return ui.screenH - rawMy
    end
end

function on_create(self)
    updateUISize()
end

function on_update(self, dt)
    -- Keep it robust for resize/orientation changes
    updateUISize()
end
