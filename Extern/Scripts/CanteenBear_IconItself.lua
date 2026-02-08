----------------------------------------------------------------
-- CanteenBear_IconItself.lua  (UPDATED)
-- Attach to the ICON UI entity (must have EntityUI)
--
-- Icon shows only if:
--   - in range: SHARED.INTERACT_IN_ZONE[SOFTTOY_ID] == true
--   - entity 378 scale != (0,0,0)
--   - (optional) interface not open
----------------------------------------------------------------

print("[CanteenBear_IconItself] ===== SCRIPT FILE LOADED =====")

local DEBUG = false

-- Set this to the canteen bear interactable id (InteractionBox ENTER/Periodic id)
local SOFTTOY_ID = 376   -- <-- CHANGE if your canteen bear id isn't 376

-- Extra requirement entity
local SCALE_CHECK_ENTITY_ID = 376

-- Hide icon while interface open?
local HIDE_WHEN_UI_OPEN = true
local UI_TABLE = "CANTEEN_BEAR_UI_OPEN"

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
        and SHARED.INTERACT_IN_ZONE[SOFTTOY_ID] == true
end

local function ui_open()
    if not HIDE_WHEN_UI_OPEN then return false end
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][SOFTTOY_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[CanteenBear_IconItself] ERROR: No UIComponent on entity", EntityID,
              "(attach to the UI icon entity, not the trigger)")
        return
    end

    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)

    if DEBUG then
        print("[CanteenBear_IconItself] init icon=", EntityID, " softtoy=", SOFTTOY_ID)
    end
end

function on_update(self, dt)
    if not UI then return end
    local show = in_range() and scale_ok() and (not ui_open())
    UI:SetVisible(show)
end
