----------------------------------------------------------------
-- PlayroomExplosionToy_TriggerBox.lua
-- Attach to the Explosion Toy interactable trigger (Entity 378)
--
-- InteractionBox sends:
--   E -> sent toggle to door 378
-- so we consume SHARED.DOOR_CMD[378] and toggle our UI open flag.
--
-- Also ensures menu is displayed via UI_DisplayMenu("ExplosionToyInterface")
--
-- Extra requirement:
--   only allow open if entity 378 scale != (0,0,0)
----------------------------------------------------------------

print("[ExplosionToy_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local TOY_ID = 378
local MENU_NAME = "ExplosionToyInterface"

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "EXPLOSION_TOY_UI_OPEN"

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

local function scale_ok()
    local tr = Query.GetTransform(TOY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    local eps = 0.01
    return not (sx < eps and sy < eps and sz < eps)
end

local function consume_toggle(id)
    if SHARED == nil then return false end
    local t = SHARED[DOOR_CMD_TABLE]
    if t == nil then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[UI_TABLE][TOY_ID] = false

    hasPlayedDiscovery = false

    if DEBUG then
        print("[ExplosionToy_TriggerBox] created. TOY_ID=", TOY_ID, " MENU=", MENU_NAME)
    end
end

function on_update(self, dt)
    if not consume_toggle(TOY_ID) then
        return
    end

    if DEBUG then
        print("[ExplosionToy_TriggerBox] Interact received for", TOY_ID)
    end

    -- Scale gate
    if not scale_ok() then
        SHARED[UI_TABLE][TOY_ID] = false
        if DEBUG then print("[ExplosionToy_TriggerBox] scale=0 -> force close UI") end
        return
    end

    -- Ensure menu is shown/loaded
    if UI_DisplayMenu ~= nil then
        UI_DisplayMenu(MENU_NAME)
    else
        print("[ExplosionToy_TriggerBox] ERROR: UI_DisplayMenu binding missing")
    end

    -- Toggle open flag
    local wasOpen = (SHARED[UI_TABLE][TOY_ID] == true)
    local isNowOpen = not wasOpen
    SHARED[UI_TABLE][TOY_ID] = isNowOpen

    if isNowOpen then
        play_3d_sfx(SOFT_TOY_PICKUP_SFX_ID)

        if not hasPlayedDiscovery then
            play_3d_sfx(SOFT_TOY_DISCOVERY_SFX_ID)
            hasPlayedDiscovery = true
        end

        if DEBUG then
            print("[ExplosionToy_TriggerBox] Toy picked up")
        end
    end

    if DEBUG then
        print("[ExplosionToy_TriggerBox] UI open =", tostring(SHARED[UI_TABLE][TOY_ID]))
    end
end