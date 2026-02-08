----------------------------------------------------------------
-- CanteenBearInterface_Root.lua
-- Attach to UI root entity INSIDE the menu (e.g., CanteenBearInterface)
-- Must have EntityUI
--
-- Visibility driven by:
--   SHARED.CANTEEN_BEAR_UI_OPEN[SOFTTOY_ID] == true
-- AND:
--   entity 378 scale != (0,0,0)
----------------------------------------------------------------

print("[CanteenBearInterface_Root] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- Must match the interactable id you toggle from
local SOFTTOY_ID = 376  -- <-- CHANGE if needed

local SCALE_CHECK_ENTITY_ID = 376
local UI_TABLE = "CANTEEN_BEAR_UI_OPEN"

local UI = nil

local function scale_ok()
    local tr = Query.GetTransform(SCALE_CHECK_ENTITY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][SOFTTOY_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[CanteenBearInterface_Root] ERROR: No UIComponent on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)

    if DEBUG then
        print("[CanteenBearInterface_Root] Initialized. uiEntity=", EntityID, " softtoy=", SOFTTOY_ID)
    end
end

function on_update(self, dt)
    if not UI then return end
    UI:SetVisible(want_open() and scale_ok())
end
