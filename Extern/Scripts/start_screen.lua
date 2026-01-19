-- ===============================================================
-- start_screen.lua
-- "Press anywhere to start" overlay, hidden via UI:SetVisible(false)
-- ===============================================================

local started = false
local UI      = nil

-- Pixel 6 Pro landscape resolution
local SCREEN_W = 2340
local SCREEN_H = 1080

-- ---------------------------------------------------------------
-- on_create : setup + center the UI
-- ---------------------------------------------------------------
function on_create(self)
    UI = EntityUI
    if not UI then
        print("[start_screen] ERROR: No UIComponent on entity", EntityID)
        return
    end

    -- Use existing size from inspector, otherwise default to full screen
    local w, h = UI:GetSize()
    if w == 0 or h == 0 then
        w, h = SCREEN_W, SCREEN_H
        UI:SetSize(w, h)
    end

    -- Pivot is top-left, so to center:
    local x = (SCREEN_W - w) * 0.5
    local y = (SCREEN_H - h) * 0.5
    UI:SetPosition(x, y)

    -- Make sure it's visible at the start
    UI:SetVisible(true)
    started = false

    print("[start_screen] Initialized, size=", w, h, "pos=", x, y)
end

-- ---------------------------------------------------------------
-- on_update : hide once player presses / clicks anywhere
-- ---------------------------------------------------------------
function on_update(self, dt)
    if started or not UI then
        return
    end

    -- Mirror how other scripts read input (dot calls, GLFW_* constants)
    local anyKey =
        Input.GetKeyDown(GLFW_KEY_W) or
        Input.GetKeyDown(GLFW_KEY_A) or
        Input.GetKeyDown(GLFW_KEY_S) or
        Input.GetKeyDown(GLFW_KEY_D) or
        Input.GetKeyDown(GLFW_KEY_E) or
        Input.GetKeyDown(GLFW_KEY_SPACE)

    local anyClick = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)

    -- If you also want touch to count on Android and it exists:
    local anyTouch = false
    if Input.WasJustTouched then
        anyTouch = Input.WasJustTouched()
    end

    if anyKey or anyClick or anyTouch then
        print("[start_screen] Input detected, hiding start overlay")
        started = true
        UI:SetVisible(false)   -- just hide UI, entity + scripts stay alive
    end
end
