----------------------------------------------------------------
-- CanteenBear_IconUI.lua
-- Attach to the ICON UI entity (must have EntityUI)
--
-- Shows when:
--   - player is in range of softtoy (SHARED.INTERACT_IN_ZONE[SOFTTOY_ID] == true)
--   - and softtoy is NOT held (optional flag)
--
-- No inventory needed.
-- When you "put it back down", just set HELD=false and icon will appear again if in range.
----------------------------------------------------------------

local DEBUG = true

-- IMPORTANT: set this to your softtoy/interactable ID (from logs: 376)
local SOFTTOY_ID = 376

-- Optional held state:
-- When picked up:  SHARED.SOFTTOY_HELD[SOFTTOY_ID] = true
-- When dropped:   SHARED.SOFTTOY_HELD[SOFTTOY_ID] = false (or nil)
local HELD_TABLE = "SOFTTOY_HELD"

local function set_visible(v)
    if EntityUI == nil then
        print("[CanteenBear_IconUI] ERROR: EntityUI is nil on icon EntityID " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[SOFTTOY_ID] == true
end

local function is_held()
    return SHARED
        and SHARED[HELD_TABLE]
        and SHARED[HELD_TABLE][SOFTTOY_ID] == true
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[HELD_TABLE] = SHARED[HELD_TABLE] or {}

    set_visible(false)

    if DEBUG then
        print("[CanteenBear_IconUI] create icon=", EntityID, " softtoy=", SOFTTOY_ID)
    end
end

function on_update(self, dt)
    -- Hide if held; else show only when in range
    if is_held() then
        set_visible(false)
        return
    end

    set_visible(in_range())
end
