----------------------------------------------------------------
-- BookInterface_Root.lua
-- Attach to UI ROOT entity inside menu "BookInterface"
-- Must have EntityUI.
--
-- Shows/hides interface based on:
--   SHARED.BOOK_UI_OPEN[BOOK_ID]
--
-- Captures UI input ONLY on transitions:
--   when open  -> cursor visible + unlocked
--   when closed -> cursor hidden + locked
-- IMPORTANT: does NOT force cursor state on_create
----------------------------------------------------------------

print("[BookInterface_Root] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local BOOK_ID  = 431          -- <<<<<< CHANGE THIS
local UI_TABLE = "BOOK_UI_OPEN"

local UI = nil
local uiWasOpen = nil -- nil = uninitialized (prevents forcing cursor lock at startup)

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][BOOK_ID] == true
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
        print("[BookInterface_Root] ERROR: No UIComponent on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    if SHARED[UI_TABLE][BOOK_ID] == nil then
        SHARED[UI_TABLE][BOOK_ID] = false
    end

    -- Start hidden, but DO NOT touch cursor/lock here
    UI:SetVisible(false)

    if DEBUG then
        print("[BookInterface_Root] Initialized. UI entity=", EntityID, " bookId=", BOOK_ID,
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
            print("[BookInterface_Root] first update, uiWasOpen=", tostring(uiWasOpen))
        end
        return
    end

    -- Only on transitions do we change cursor/lock
    if open ~= uiWasOpen then
        set_ui_input(open)
        uiWasOpen = open
        if DEBUG then
            print("[BookInterface_Root] UI open changed ->", tostring(open))
        end
    end
end

function on_destroy(self)
    -- Do NOT force cursor/lock here either
end
