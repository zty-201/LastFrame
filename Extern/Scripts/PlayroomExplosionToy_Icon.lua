----------------------------------------------------------------
-- ExplosionToy_IconUI.lua
-- Attach to ICON UI entity (must have EntityUI)
--
-- Shows icon when:
--   - in range: SHARED.INTERACT_IN_ZONE[378] == true
--   - entity 378 scale != (0,0,0)
--   - optional: hide icon if interface open
----------------------------------------------------------------

print("[ExplosionToy_IconUI] ===== SCRIPT FILE LOADED =====")

local TOY_ID = 378
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"
local HIDE_WHEN_UI_OPEN = true

local UI = nil

local function scale_ok()
    local tr = Query.GetTransform(TOY_ID)
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
        print("[ExplosionToy_IconUI] ERROR: No UIComponent 'EntityUI' on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)
end

function on_update(self, dt)
    if not UI then return end
    UI:SetVisible(in_range() and scale_ok() and (not ui_open()))
end
