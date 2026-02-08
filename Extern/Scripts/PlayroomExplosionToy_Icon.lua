----------------------------------------------------------------
-- PlayroomToy_IconUI.lua
-- Attach to the ICON UI entity (must have EntityUI)
--
-- Shows only if:
--   1) SHARED.PLAYROOM_TOY_ICON_SHOW[TOY_ID] == true
--   2) Entity 378 scale is NOT (0,0,0)
----------------------------------------------------------------

local DEBUG = false

-- Set this to the trigger/toy id (the one InteractionBox ENTER reports)
local TOY_ID = 378

-- Your additional requirement entity
local SCALE_CHECK_ENTITY_ID = 378

local IN_TABLE = "PLAYROOM_TOY_ICON_SHOW"

-- prevent log spam
local printedNoUI = false

local function set_visible(v)
    if EntityUI == nil then
        if not printedNoUI then
            print("[PlayroomToy_IconUI] ERROR: EntityUI is nil on icon EntityID " .. tostring(EntityID) ..
                  " (You attached this to a non-UI entity. Attach it to the UI icon entity.)")
            printedNoUI = true
        end
        return
    end
    EntityUI:SetVisible(v)
end

local function toy_should_show()
    if SHARED == nil then return false end
    local t = SHARED[IN_TABLE]
    if t == nil then return false end
    return t[TOY_ID] == true
end

local function scale_check_ok()
    local tr = Query.GetTransform(SCALE_CHECK_ENTITY_ID)
    if tr == nil then return false end

    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[IN_TABLE] = SHARED[IN_TABLE] or {}

    set_visible(false)

    if DEBUG then
        print("[PlayroomToy_IconUI] create icon=", EntityID, " toyId=", TOY_ID, " scaleCheck=", SCALE_CHECK_ENTITY_ID)
    end
end

function on_update(self, dt)
    if TOY_ID == 0 then
        set_visible(false)
        return
    end

    local show = toy_should_show() and scale_check_ok()
    set_visible(show)
end
