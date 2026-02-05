-- DoorUnlockIcon_UI.lua
-- Attach to ICON entity (607).
-- Shows only when door is in range AND door has NOT opened once.

local DOOR_ID = 26

local function set_visible(v)
    if EntityUI == nil then
        print("[DoorUnlockIcon_UI] EntityUI is nil on icon entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function door_has_opened_once()
    return SHARED.DOOR_OPENED_ONCE ~= nil and SHARED.DOOR_OPENED_ONCE[DOOR_ID] == true
end

function on_create(self)
    SHARED.ICON_CMD = SHARED.ICON_CMD or {}
    SHARED.DOOR_OPENED_ONCE = SHARED.DOOR_OPENED_ONCE or {}
    -- default if door didn't init it yet
    if SHARED.DOOR_OPENED_ONCE[DOOR_ID] == nil then
        SHARED.DOOR_OPENED_ONCE[DOOR_ID] = false
    end

    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.ICON_CMD and SHARED.ICON_CMD[DOOR_ID]
    if not msg then return end

    if msg.cmd == "show" then
        -- ✅ only show if the door has never opened once
        if not door_has_opened_once() then
            set_visible(true)
        else
            set_visible(false)
        end
    elseif msg.cmd == "hide" then
        set_visible(false)
    end

    SHARED.ICON_CMD[DOOR_ID] = nil
end
