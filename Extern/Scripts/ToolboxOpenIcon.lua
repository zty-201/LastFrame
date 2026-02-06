-- TriggerIcon_UI.lua
-- Attach to the ICON UI entity
-- Shows when TRIGGER_ICON_CMD[TARGET_ID] says show (armed)
-- and player is in range.
-- Hides forever after trigger is used (script will receive hide permanently).

local TARGET_ID = 479   -- <-- set to the interactable/trigger id

local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[TriggerIcon_UI] ERROR: EntityUI nil on " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

function on_create(self)
    SHARED.TRIGGER_ICON_CMD = SHARED.TRIGGER_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    armed = false
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.TRIGGER_ICON_CMD and SHARED.TRIGGER_ICON_CMD[TARGET_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.TRIGGER_ICON_CMD[TARGET_ID] = nil
    end

    if armed then
        set_visible(in_range())
    end
end
