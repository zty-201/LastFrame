----------------------------------------------------------------
-- CanteenSofttoy_IconHover.lua
-- Attach to the CANTEEN SOFTOY entity (or its interactable trigger entity)
--
-- Goal: icon hover mechanic only (show icon when in range, hide when not)
-- Uses: SHARED.INTERACT_IN_ZONE[EntityID] == true (set by your InteractionBox)
-- Outputs: SHARED.SOFTTOY_ICON_CMD[EntityID] = { cmd="show"/"hide" }
----------------------------------------------------------------

local DEBUG = true

-- If your interactable trigger entity is NOT the same entity this script is on,
-- set this to that entity id. Otherwise leave as 0 (auto = EntityID).
local TARGET_ENTITY_ID = 0

-- Command bus name (match whatever your icon display script listens to)
-- If you already have an icon script listening on KEY_ICON_CMD, you can set this to "KEY_ICON_CMD"
local ICON_CMD_TABLE_NAME = "SOFTTOY_ICON_CMD"

-- Internal
local function send_icon_cmd(id, cmd)
    if SHARED == nil then return end
    SHARED[ICON_CMD_TABLE_NAME] = SHARED[ICON_CMD_TABLE_NAME] or {}
    SHARED[ICON_CMD_TABLE_NAME][id] = { cmd = cmd, by = id }
end

local function in_range(id)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[id] == true
end

function on_create(Self)
    if DEBUG then
        print("[Softtoy_IconHover] on_create EntityID:", EntityID)
        print("[Softtoy_IconHover] ICON_CMD_TABLE_NAME:", ICON_CMD_TABLE_NAME)
    end

    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[ICON_CMD_TABLE_NAME] = SHARED[ICON_CMD_TABLE_NAME] or {}
end

function on_update(Self, dt)
    local softtoyId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    if in_range(softtoyId) then
        send_icon_cmd(softtoyId, "show")
    else
        send_icon_cmd(softtoyId, "hide")
    end
end
