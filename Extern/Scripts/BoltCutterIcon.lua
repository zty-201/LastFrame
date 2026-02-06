-- BoltCutterIcon_UI.lua
-- Attach to BOLTCUTTER ICON UI entity
-- Shows when KEY_ICON_CMD[TARGET_ID] says show (armed)
-- and player is in range.
-- After pickup script sends hide, icon stays off.

local TARGET_ID = 481  -- <-- set to the boltcutter pickup interactable id
local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[BoltCutterIcon_UI] ERROR: EntityUI nil on " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

function on_create(self)
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    armed = false
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.KEY_ICON_CMD and SHARED.KEY_ICON_CMD[TARGET_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.KEY_ICON_CMD[TARGET_ID] = nil
    end

    if armed then
        set_visible(in_range())
    end
end
