----------------------------------------------------------------
-- PlayroomExplosionToy_TriggerBox.lua
-- Attach to the Explosion Toy interactable trigger (Entity 378)
--
-- InteractionBox behavior (from your logs):
--   Press E -> "sent toggle to door 378"
-- So we consume SHARED.DOOR_CMD[378] as our "interact" trigger.
--
-- When interacted:
--   - ensure menu "ExplosionToyInterface" is loaded/opened
--   - toggle SHARED.EXPLOSION_TOY_UI_OPEN[378]
--
-- UI will only open if entity 378 scale != (0,0,0)
----------------------------------------------------------------

local DEBUG = true

local TOY_ID = EntityID
local MENU_NAME = "ExplosionToyInterface"

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "EXPLOSION_TOY_UI_OPEN"

local function scale_ok()
    local tr = Query.GetTransform(TOY_ID)
    if tr == nil then return false end
    local sx, sy, sz = tr:GetScale()
    return not (sx == 0 and sy == 0 and sz == 0)
end

local function consume_toggle(id)
    if SHARED == nil then return false end
    local t = SHARED[DOOR_CMD_TABLE]
    if t == nil then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

-- Try to open/load menu using whatever binding exists in your engine.
local function open_menu(menuName)
    -- If your engine exposes one of these, we’ll use it.
    local candidates = {
        "UI_OpenMenu",
        "UI_LoadMenu",
        "UI_ShowMenu",
        "Menu_Open",
        "Menu_Load",
        "MenuManager_Open",
        "MenuManager_Load",
    }

    for i = 1, #candidates do
        local fnName = candidates[i]
        local fn = _G[fnName]
        if fn ~= nil then
            local ok, err = pcall(fn, menuName)
            if ok then
                if DEBUG then print("[ExplosionToy_Trigger] Opened menu via", fnName, "->", menuName) end
                return true
            else
                print("[ExplosionToy_Trigger] Found", fnName, "but call failed:", tostring(err))
            end
        end
    end

    print("[ExplosionToy_Trigger] ERROR: No menu open/load binding found for", menuName)
    print("[ExplosionToy_Trigger] TIP: tell me the real function name you use to open menus (e.g., UI_OpenMenu)")
    return false
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    -- start closed
    SHARED[UI_TABLE][TOY_ID] = false

    if DEBUG then
        print("[ExplosionToy_Trigger] created. TOY_ID=", TOY_ID, " MENU=", MENU_NAME)
    end
end

function on_update(self, dt)
    -- Only respond when E produced a toggle for this interactable
    if not consume_toggle(TOY_ID) then
        return
    end

    if DEBUG then
        print("[ExplosionToy_Trigger] Interact received for", TOY_ID)
    end

    -- Extra requirement: if toy is hidden by scale 0,0,0, do nothing (and force close)
    if not scale_ok() then
        SHARED[UI_TABLE][TOY_ID] = false
        if DEBUG then print("[ExplosionToy_Trigger] scale=0 -> force close UI") end
        return
    end

    -- Make sure menu exists/loaded
    open_menu(MENU_NAME)

    -- Toggle UI open flag
    SHARED[UI_TABLE][TOY_ID] = not (SHARED[UI_TABLE][TOY_ID] == true)

    if DEBUG then
        print("[ExplosionToy_Trigger] UI open =", tostring(SHARED[UI_TABLE][TOY_ID]))
    end
end
