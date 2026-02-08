----------------------------------------------------------------
-- ExplosionToyInterface_Root.lua
-- Attach to the UI root entity INSIDE the ExplosionToyInterface menu
-- (This entity must have EntityUI)
--
-- Shows/hides based on:
--   SHARED.EXPLOSION_TOY_UI_OPEN[378] == true
-- AND extra requirement:
--   entity 378 scale != (0,0,0)
----------------------------------------------------------------

local DEBUG = true

local TOY_ID = 378
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"

local UI = nil

local function scale_ok()
    local tr = Query.GetTransform(TOY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function want_open()
    if SHARED == nil then return false end
    local t = SHARED[UI_TABLE]
    if t == nil then return false end
    return t[TOY_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ExplosionToyInterface] ERROR: No UIComponent on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    -- start hidden
    UI:SetVisible(false)

    if DEBUG then
        print("[ExplosionToyInterface] Initialized. UI entity=", EntityID, " toyId=", TOY_ID)
    end
end

function on_update(self, dt)
    if not UI then return end

    local show = want_open() and scale_ok()
    UI:SetVisible(show)
end
