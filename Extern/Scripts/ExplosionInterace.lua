----------------------------------------------------------------
-- ExplosionToyInterface_Root.lua
-- Attach to UI ROOT entity inside menu "ExplosionToyInterface"
-- Must have EntityUI.
--
-- Shows/hides interface based on:
--   SHARED.EXPLOSION_TOY_UI_OPEN[378]
--
-- Captures UI input ONLY on transitions:
--   when open  -> cursor visible + unlocked
--   when closed -> cursor visible + unlocked (game uses hold-to-look, cursor always visible)
-- IMPORTANT: does NOT force cursor state on_create (prevents cursor disappearing at game start)
----------------------------------------------------------------

print("[ExplosionToyInterface_Root] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local TOY_ID = 378
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"

local UI = nil

-- nil = uninitialized (prevents forcing cursor lock at startup)
local uiWasOpen = nil

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

local function set_ui_input(open)
    -- Cursor is always visible + unlocked in this game (camera uses hold-to-look via RMB).
    -- player_pov_camera only changes cursor on RMB state transitions, so it won't
    -- restore cursor after a close. We must always restore to visible+unlocked here.
    if Input ~= nil and Input.SetCursorVisible ~= nil then
        Input.SetCursorVisible(true)
    end
    if Input ~= nil and Input.SetCursorLocked ~= nil then
        Input.SetCursorLocked(false)
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
    if SHARED[UI_TABLE][TOY_ID] == nil then
        SHARED[UI_TABLE][TOY_ID] = false
    end

    -- Start hidden, but DO NOT touch cursor/lock here
    UI:SetVisible(false)

    if DEBUG then
        print("[ExplosionToyInterface_Root] Initialized. UI entity=", EntityID, " toyId=", TOY_ID,
              "(not forcing cursor/lock on_create)")
    end
end

function on_update(self, dt)
    if not UI then return end

    local open = want_open()
    UI:SetVisible(open)

    -- First update: initialize state WITHOUT forcing cursor changes
    if uiWasOpen == nil then
        uiWasOpen = open
        if DEBUG then
            print("[ExplosionToyInterface_Root] first update, uiWasOpen=", tostring(uiWasOpen))
        end
        return
    end

    -- Only on transitions do we change cursor/lock
    if open ~= uiWasOpen then
        set_ui_input(open)
        uiWasOpen = open
        if DEBUG then
            print("[ExplosionToyInterface_Root] UI open changed ->", tostring(open))
        end
    end
end

function on_destroy(self)
    -- Do NOT force cursor/lock here either
    -- (prevents random cursor state changes during menu teardown)
end
