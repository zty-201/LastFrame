-- DoorLockIcon_UI.lua
-- Attach to the LOCK ICON UI entity.
-- Starts invisible.
-- Shows only when:
--   1) we have received LOCK_ICON_CMD[DOOR_ID] = "show" (armed)
--   2) door is in interaction range (SHARED.INTERACT_IN_ZONE[DOOR_ID] == true)

local DOOR_ID = 26   -- door this lock icon is for

local armed = false  -- ✅ NEW: remembers "show requested" until in range

local function set_visible(v)
    if EntityUI == nil then
        print("[DoorLockIcon_UI] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function door_in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[DOOR_ID] == true
end

function on_create(self)
    SHARED.LOCK_ICON_CMD = SHARED.LOCK_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {} -- ✅ NEW safeguard
    set_visible(false) -- keep invisible until triggered
end

function on_update(self, dt)
    -- ✅ NEW: consume lock-icon command, but gate by range
    local msg = SHARED.LOCK_ICON_CMD and SHARED.LOCK_ICON_CMD[DOOR_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.LOCK_ICON_CMD[DOOR_ID] = nil -- consume command
    end

    -- ✅ NEW: if armed, only show when in range
    if armed then
        set_visible(door_in_range())
    end
end
