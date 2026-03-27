print("[CanteenToy_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- MUST match InteractionBox ID for the canteen toy
local TOY_ID = 376
local CANTEEN_DOOR_ID = 400

local MENU_NAME = "CanteenToyInterface"

local DOOR_CMD_TABLE   = "DOOR_CMD"
local UI_TABLE         = "CANTEEN_TOY_UI_OPEN"
local LIGHTS_CMD_TABLE = "LIGHTS_CMD"

local SOFT_TOY_PICKUP_SFX_ID    = "SoftToyPickup"
local SOFT_TOY_DISCOVERY_SFX_ID = "Discovery"

local hasPlayedDiscovery = false

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[LIGHTS_CMD_TABLE] = SHARED[LIGHTS_CMD_TABLE] or {}

    SHARED[UI_TABLE][TOY_ID] = false

    hasPlayedDiscovery = false

    if DEBUG then
        print("[CanteenToy_TriggerBox] created TOY_ID=", TOY_ID, " MENU=", MENU_NAME)
    end
end

local function consume_toggle(id)
    local t = SHARED and SHARED[DOOR_CMD_TABLE]
    if not t then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

-- Shared put-down logic: close UI, unlock canteen door, restore lights
local function put_down_toy()
    SHARED[UI_TABLE][TOY_ID] = false

    SHARED[DOOR_CMD_TABLE][CANTEEN_DOOR_ID] = {
        cmd = "unlock",
        by = EntityID
    }

    SHARED[LIGHTS_CMD_TABLE]["CANTEEN_LIGHTS_ON"]       = true
    SHARED[LIGHTS_CMD_TABLE]["CANTEEN_LIGHTS_OFF"]      = false
    SHARED[LIGHTS_CMD_TABLE]["CANTEEN_LIGHTS_DISABLED"] = false

    if UI_DisplayMenu then UI_DisplayMenu(MENU_NAME) end

    if DEBUG then
        print("[CanteenToy_TriggerBox] Toy put down -> unlocking canteen door, lights on")
    end
end

function on_update(self, dt)
    -- Auto-close (put down) if player walked out of the zone while holding the toy
    if SHARED[UI_TABLE][TOY_ID] == true
       and not (SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[TOY_ID] == true) then
        if DEBUG then print("[CanteenToy_TriggerBox] Auto put-down: player left zone") end
        put_down_toy()
    end

    if not consume_toggle(TOY_ID) then return end

    local wasOpen = (SHARED[UI_TABLE][TOY_ID] == true)
    local isNowOpen = not wasOpen
    SHARED[UI_TABLE][TOY_ID] = isNowOpen

    if isNowOpen then
        -- PICK UP TOY
        if UI_DisplayMenu then
            UI_DisplayMenu(MENU_NAME)
        else
            print("[CanteenToy_TriggerBox] ERROR: UI_DisplayMenu missing")
        end

        play_3d_sfx(SOFT_TOY_PICKUP_SFX_ID)

        if not hasPlayedDiscovery then
            play_3d_sfx(SOFT_TOY_DISCOVERY_SFX_ID)
            hasPlayedDiscovery = true
        end

        if DEBUG then
            print("[CanteenToy_TriggerBox] Toy picked up")
            print("[CanteenToy_TriggerBox] UI_OPEN =", tostring(SHARED[UI_TABLE][TOY_ID]))
        end
    else
        -- PUT DOWN TOY manually
        put_down_toy()
        if DEBUG then
            print("[CanteenToy_TriggerBox] UI_OPEN =", tostring(SHARED[UI_TABLE][TOY_ID]))
        end
    end
end