print("[CanteenToyInterface_Root] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local TOY_ID   = 376
local UI_TABLE = "CANTEEN_TOY_UI_OPEN"

local UI = nil

-- IMPORTANT:
-- nil = not initialized yet (prevents forcing cursor lock at game start)
local uiWasOpen = nil

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

local function set_ui_input(open)
    -- When UI opens:
    --   show cursor + unlock
    -- When UI closes:
    --   hide cursor + lock
    if Input and Input.SetCursorVisible then
        Input.SetCursorVisible(open)
    end
    if Input and Input.SetCursorLocked then
        Input.SetCursorLocked(not open)
    end
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[CanteenToyInterface_Root] ERROR: No EntityUI on", EntityID)
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
        print("[CanteenToyInterface_Root] init. TOY_ID=", TOY_ID, "UI_TABLE=", UI_TABLE,
              " (not forcing cursor/lock on_create)")
    end
end

function on_update(self, dt)
    if not UI then return end

    local open = want_open()
    UI:SetVisible(open)

    -- First update: initialize state WITHOUT forcing cursor changes.
    if uiWasOpen == nil then
        uiWasOpen = open
        if DEBUG then
            print("[CanteenToyInterface_Root] first update, uiWasOpen=", tostring(uiWasOpen))
        end
        return
    end

    -- Only on transitions do we change cursor/lock
    if open ~= uiWasOpen then
        set_ui_input(open)
        uiWasOpen = open
        if DEBUG then
            print("[CanteenToyInterface_Root] UI open changed ->", tostring(open))
        end
    end
end

function on_destroy(self)
    -- Do NOT force cursor here either (prevents startup/teardown weirdness)
end
