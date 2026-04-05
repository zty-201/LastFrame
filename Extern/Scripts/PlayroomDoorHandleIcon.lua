-- PlayroomDoorHandleIcon_UI.lua
-- Attach to DOOR HANDLE ICON UI entity

local DOOR_ID = 33
local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[PlayroomDoorHandleIcon_UI] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function door_in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[DOOR_ID] == true
end

function on_create(self)
    armed = false
    SHARED.HANDLE_ICON_CMD = SHARED.HANDLE_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.HANDLE_ICON_CMD and SHARED.HANDLE_ICON_CMD[DOOR_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.HANDLE_ICON_CMD[DOOR_ID] = nil
    end

    if armed then
        set_visible(door_in_range())
    end
end