-- PlayroomKeyPickupIcon_UI.lua
-- Attach to the KEY PICKUP icon UI entity.
-- Shows only when:
--   1) KEY_ICON_CMD[KEY_ID] says "show" (armed)
--   2) key is in interaction range (SHARED.INTERACT_IN_ZONE[KEY_ID] == true)

local KEY_ID = 218   -- TODO: set this to the key interactable entity id (from logs)
local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[PlayroomKeyPickupIcon_UI] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function key_in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[KEY_ID] == true
end

function on_create(self)
    armed = false
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.KEY_ICON_CMD and SHARED.KEY_ICON_CMD[KEY_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.KEY_ICON_CMD[KEY_ID] = nil
    end

    if armed then
        set_visible(key_in_range())
    end
end
