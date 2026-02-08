----------------------------------------------------------------
-- ExplosionToyInterface_Root.lua
-- Attach to UI ROOT entity inside menu "ExplosionToyInterface"
-- Must have EntityUI.
--
-- Shows/hides interface based on:
--   SHARED.EXPLOSION_TOY_UI_OPEN[378]
-- Also enforces:
--   entity 378 scale != (0,0,0)
--
-- IMPORTANT: Captures UI input:
--   when open -> cursor visible + unlocked
--   when closed -> cursor hidden + locked
----------------------------------------------------------------

print("[ExplosionToyInterface_Root] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local TOY_ID = 378
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"

local UI = nil
local uiWasOpen = false

local function scale_ok()
    local tr = Query.GetTransform(TOY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

local function set_ui_input(open)
    if Input ~= nil and Input.SetCursorVisible ~= nil then
        Input.SetCursorVisible(open)
    end
    if Input ~= nil and Input.SetCursorLocked ~= nil then
        Input.SetCursorLocked(not open) -- unlock when UI open
    end
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ExplosionToyInterface_Root] ERROR: No UIComponent on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)
    uiWasOpen = false
    set_ui_input(false)

    if DEBUG then
        print("[ExplosionToyInterface_Root] Initialized. UI entity=", EntityID, " toyId=", TOY_ID)
    end
end

function on_update(self, dt)
    if not UI then return end

    local open = want_open() and scale_ok()
    UI:SetVisible(open)

    if open ~= uiWasOpen then
        set_ui_input(open)
        uiWasOpen = open
        if DEBUG then
            print("[ExplosionToyInterface_Root] UI open changed ->", tostring(open))
        end
    end
end

function on_destroy(self)
    -- Safety: if UI entity is destroyed while open, restore gameplay input
    set_ui_input(false)
end
