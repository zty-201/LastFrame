-- Playroom_CanteenDoorSlamTrigger.lua
-- Slam the canteen door shut and lock it when player enters this trigger
-- BUT only after the hidden wall has already been removed.

local DEBUG = true
local PLAYER_ENTITY_ID = 5

local WALL_ENTITY_ID = 43
local CANTEEN_DOOR_ID = 400

local fired = false
local playerInsideThisZone = false

local function wall_is_hidden()
    local tr = Query.GetTransform(WALL_ENTITY_ID)
    if tr == nil then return false end

    local sx, sy, sz = tr:GetScale()
    return math.abs(sx) < 0.001 and math.abs(sy) < 0.001 and math.abs(sz) < 0.001
end

local function trigger_slam()
    if fired then return end

    if not wall_is_hidden() then
        if DEBUG then
            print("[Playroom_CanteenDoorSlamTrigger] Wall not hidden yet, slam ignored")
        end
        return
    end

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.LIGHTS_CMD = SHARED.LIGHTS_CMD or {}

    SHARED.DOOR_CMD[CANTEEN_DOOR_ID] = {
        cmd = "close_and_lock",
        by = EntityID
    }

    -- Turn off canteen lights
    SHARED.LIGHTS_CMD["CANTEEN_LIGHTS_OFF"] = true
    SHARED.LIGHTS_CMD["CANTEEN_LIGHTS_ON"] = false

    fired = true

    if DEBUG then
        print("[Playroom_CanteenDoorSlamTrigger] Triggered canteen door CLOSE_AND_LOCK")
        print("[Playroom_CanteenDoorSlamTrigger] Triggered CANTEEN_LIGHTS_OFF")
    end
end

function on_create(self)
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.LIGHTS_CMD = SHARED.LIGHTS_CMD or {}

    fired = false
    playerInsideThisZone = false

    SHARED.LIGHTS_CMD["CANTEEN_LIGHTS_OFF"] = false
    SHARED.LIGHTS_CMD["CANTEEN_LIGHTS_ON"] = false
    SHARED.LIGHTS_CMD["CANTEEN_LIGHTS_DISABLED"] = false

    if DEBUG then
        print(string.format(
            "[Playroom_CanteenDoorSlamTrigger] on_create | Zone=%d | DoorID=%d | WallID=%d",
            EntityID, CANTEEN_DOOR_ID, WALL_ENTITY_ID
        ))
    end
end

function on_trigger_enter(self, otherID)
    if DEBUG then
        print(string.format(
            "[Playroom_CanteenDoorSlamTrigger] Trigger ENTER | Zone=%d | Other=%d",
            EntityID, otherID
        ))
    end

    if otherID ~= PLAYER_ENTITY_ID then
        if DEBUG then
            print("[Playroom_CanteenDoorSlamTrigger] ENTER ignored: not player")
        end
        return
    end

    if playerInsideThisZone then
        if DEBUG then
            print("[Playroom_CanteenDoorSlamTrigger] Already inside this zone. Debounced.")
        end
        return
    end

    playerInsideThisZone = true
    trigger_slam()
end

function on_trigger_exit(self, otherID)
    if DEBUG then
        print(string.format(
            "[Playroom_CanteenDoorSlamTrigger] Trigger EXIT | Zone=%d | Other=%d",
            EntityID, otherID
        ))
    end

    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    playerInsideThisZone = false
end

function on_destroy(self)
    playerInsideThisZone = false
end