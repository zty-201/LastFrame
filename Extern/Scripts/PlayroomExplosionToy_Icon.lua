----------------------------------------------------------------
-- PlayroomExplosionToy_IconUI.lua
-- Attach to the ICON UI entity (must have EntityUI)
--
-- Shows icon only if:
--   - player is in range (InteractionBox): SHARED.INTERACT_IN_ZONE[TOY_ID] == true
--   - entity 378 scale != (0,0,0)
-- Optional:
--   - hide icon while interface is open
----------------------------------------------------------------

local DEBUG = false

local TOY_ID = 378
local SCALE_CHECK_ENTITY_ID = 378

-- If you want icon to disappear while interface is open:
local HIDE_WHEN_UI_OPEN = true
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"

local UI = nil

local function scale_ok()
    local tr = Query.GetTransform(SCALE_CHECK_ENTITY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[TOY_ID] == true
end

local function ui_open()
    if not HIDE_WHEN_UI_OPEN then return false end
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ExplosionToy_IconUI] ERROR: No UIComponent on entity", EntityID,
              "(attach this script to the UI icon entity, not trigger 378)")
        return
    end

    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)

    if DEBUG then
        print("[ExplosionToy_IconUI] Initialized. icon=", EntityID, " toyId=", TOY_ID)
    end
end

function on_update(self, dt)
    if not UI then return end

    local show = in_range() and scale_ok() and (not ui_open())
    UI:SetVisible(show)
end
