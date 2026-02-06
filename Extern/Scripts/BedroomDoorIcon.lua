-- BedroomDoorIcon_UI.lua
-- UI icon controller:
-- Show icon ONLY when player is in range of TARGET_ID (InteractionBox zone),
-- and door has NOT been opened once yet.
-- Once opened once -> hide forever.

local TARGET_ID = 240  -- <-- set to bedroom door interactable/trigger id

local function set_visible(v)
    if EntityUI == nil then
        print("[BedroomDoorIcon_UI] ERROR: EntityUI nil on " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

function on_create(self)
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.BEDROOM_DOOR_OPENED_ONCE = SHARED.BEDROOM_DOOR_OPENED_ONCE or {}

    -- start hidden ALWAYS (prevents spawn show)
    set_visible(false)
end

function on_update(self, dt)
    -- if door opened once, hide forever
    if SHARED.BEDROOM_DOOR_OPENED_ONCE and SHARED.BEDROOM_DOOR_OPENED_ONCE[TARGET_ID] == true then
        set_visible(false)
        return
    end

    -- otherwise show only when in interaction zone
    set_visible(in_range())
end
