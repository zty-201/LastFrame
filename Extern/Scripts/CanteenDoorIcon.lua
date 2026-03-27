-- CanteenDoorIcon_UI.lua
-- Show icon only when:
-- 1. player is in interaction range
-- 2. the canteen door is currently CLOSED
-- 3. the player has not opened it once already
--
-- This means:
-- - locked closed door -> icon shows (can attempt interaction)
-- - unlocked closed door -> icon shows (can open)
-- - once opened by player -> hide forever

local TARGET_ID = 400  -- canteen door entity id

local function set_visible(v)
    if EntityUI == nil then
        print("[CanteenDoorIcon_UI] ERROR: EntityUI nil on " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil and SHARED.INTERACT_IN_ZONE[TARGET_ID] == true
end

local function is_closed()
    return SHARED.CANTEEN_DOOR_OPEN ~= nil and SHARED.CANTEEN_DOOR_OPEN[TARGET_ID] == false
end

local function opened_once()
    return SHARED.CANTEEN_DOOR_OPENED_ONCE ~= nil and SHARED.CANTEEN_DOOR_OPENED_ONCE[TARGET_ID] == true
end

function on_create(self)
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.CANTEEN_DOOR_OPEN = SHARED.CANTEEN_DOOR_OPEN or {}
    SHARED.CANTEEN_DOOR_OPENED_ONCE = SHARED.CANTEEN_DOOR_OPENED_ONCE or {}

    set_visible(false)
end

function on_update(self, dt)
    if opened_once() then
        set_visible(false)
        return
    end

    if in_range() and is_closed() then
        set_visible(true)
    else
        set_visible(false)
    end
end