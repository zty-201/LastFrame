-- MusicDiscIcon_UI.lua
-- Attach to the music disc pickup icon UI entity.
-- Shows only when:
--   1) DISC_ICON_CMD[DISC_ID] says "show" (armed)
--   2) disc is in interaction range (SHARED.INTERACT_IN_ZONE[DISC_ID] == true)

local DISC_ID = 508   -- TODO: set to the disc interactable entity id
local armed = false

local function set_visible(v)
    if EntityUI == nil then
        print("[MusicDiscIcon_UI] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function disc_in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[DISC_ID] == true
end

function on_create(self)
    SHARED.DISC_ICON_CMD = SHARED.DISC_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    set_visible(false)
end

function on_update(self, dt)
    local msg = SHARED.DISC_ICON_CMD and SHARED.DISC_ICON_CMD[DISC_ID]
    if msg then
        if msg.cmd == "show" then
            armed = true
        elseif msg.cmd == "hide" then
            armed = false
            set_visible(false)
        end
        SHARED.DISC_ICON_CMD[DISC_ID] = nil
    end

    if armed then
        set_visible(disc_in_range())
    end
end
