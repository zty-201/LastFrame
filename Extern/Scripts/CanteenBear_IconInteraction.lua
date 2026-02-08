----------------------------------------------------------------
-- CanteenBear_IconInteraction.lua  (UPDATED)
-- Attach to the CANTEEN BEAR interactable trigger entity
--
-- InteractionBox sends:
--   E -> sent toggle to door <id>
-- so we consume SHARED.DOOR_CMD[id] as our interact.
--
-- Opens menu + toggles UI flag:
--   SHARED.CANTEEN_BEAR_UI_OPEN[id] = true/false
--
-- Extra requirement:
--   Only allow open if entity 378 scale != (0,0,0)
----------------------------------------------------------------

print("[CanteenBear_IconInteraction] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- If this script is on the interactable entity itself, leave 0.
local TARGET_ENTITY_ID = 0

-- CHANGE THIS to your actual menu name in “Available Menu”
local MENU_NAME = "CanteenBearInterface"

-- Extra requirement entity
local SCALE_CHECK_ENTITY_ID = 376

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "CANTEEN_BEAR_UI_OPEN"

local function id()
    return (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID
end

local function scale_ok()
    local tr = Query.GetTransform(SCALE_CHECK_ENTITY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function consume_toggle(targetId)
    if SHARED == nil then return false end
    local t = SHARED[DOOR_CMD_TABLE]
    if t == nil then return false end
    if t[targetId] == nil then return false end
    t[targetId] = nil
    return true
end

local function open_menu(menuName)
    local candidates = {
        "UI_OpenMenu",
        "UI_LoadMenu",
        "UI_ShowMenu",
        "Menu_Open",
        "Menu_Load",
    }

    for i = 1, #candidates do
        local fn = _G[candidates[i]]
        if fn ~= nil then
            local ok, err = pcall(fn, menuName)
            if ok then
                if DEBUG then print("[CanteenBear] Opened menu:", menuName, "via", candidates[i]) end
                return true
            else
                print("[CanteenBear] Found", candidates[i], "but failed:", tostring(err))
            end
        end
    end

    print("[CanteenBear] ERROR: No menu open/load binding found. Menu=", tostring(menuName))
    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    SHARED[UI_TABLE][id()] = false

    if DEBUG then
        print("[CanteenBear] created id=", id(), " menu=", MENU_NAME, " scaleCheck=", SCALE_CHECK_ENTITY_ID)
    end
end

function on_update(self, dt)
    local targetId = id()

    if not consume_toggle(targetId) then
        return
    end

    if not scale_ok() then
        SHARED[UI_TABLE][targetId] = false
        if DEBUG then print("[CanteenBear] scale=0 -> force close UI") end
        return
    end

    open_menu(MENU_NAME)

    SHARED[UI_TABLE][targetId] = not (SHARED[UI_TABLE][targetId] == true)

    if DEBUG then
        print("[CanteenBear] UI open =", tostring(SHARED[UI_TABLE][targetId]))
    end
end
