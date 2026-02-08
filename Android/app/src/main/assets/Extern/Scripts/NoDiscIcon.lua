-- DiscUse_NoDiscIcon_UI.lua
-- Shows when DISC_USE_NODISC_ICON_CMD[TARGET_ID] says show (armed)
-- and player is in range.

local TARGET_ID = 223   -- TODO set to the interactable id this consume script listens on
local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[DiscUse_NoDiscIcon_UI] ERROR: EntityUI nil on " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

function on_create(self)
    SHARED.DISC_USE_NODISC_ICON_CMD = SHARED.DISC_USE_NODISC_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.DISC_USE_NODISC_ICON_CMD and SHARED.DISC_USE_NODISC_ICON_CMD[TARGET_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.DISC_USE_NODISC_ICON_CMD[TARGET_ID] = nil
    end

    if armed then
        set_visible(in_range())
    end
end
