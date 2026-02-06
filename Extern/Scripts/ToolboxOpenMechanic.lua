----------------------------------------------------------------
-- Trigger_MoveDown_And_Activate.lua
-- When in InteractionBox range and E is pressed:
-- 1) Move THIS entity to y = -1000
-- 2) Set entity 480 scale to 1,1,1
-- 3) Move entity 481 to (646, 69.7, 226)
--
-- ✅ Also drives a UI icon via:
--    SHARED.TRIGGER_ICON_CMD[id] = { cmd = "show"/"hide", by = ... }
----------------------------------------------------------------

local DEBUG = true

-- If the interactable id is NOT the entity this script is on,
-- set this. Otherwise leave 0 (uses EntityID).
local TARGET_ENTITY_ID = 0

local SCALE_ENTITY_ID = 480
local MOVE_ENTITY_ID  = 481

local MOVE_SELF_Y = -1000.0

local SCALE_TO = { 1.0, 1.0, 1.0 }
local MOVE_TO  = { 646.0, 69.7, 226.0 }

local triggeredOnce = false

-- ---------------------------------------
-- Helpers
-- ---------------------------------------
local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end
    cmdTable[id] = nil -- consume

    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup")
            or (c == "take") or (c == "use") or (c == "open")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "take"
        or cmd == "use" or cmd == "open" then
        return true
    end

    return true
end

local function in_range(id)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[id] == true
end

local function move_self_y(id, newY)
    local tr = Query.GetTransform(id)
    if not tr then return false end
    local x, _, z = tr:GetPosition()
    tr:SetPosition(x, newY, z)
    return true
end

local function set_scale(id, sx, sy, sz)
    local tr = Query.GetTransform(id)
    if tr then
        tr:SetScale(sx, sy, sz)
        return true
    end
    return false
end

local function set_pos(id, x, y, z)
    local tr = Query.GetTransform(id)
    if tr then
        tr:SetPosition(x, y, z)
        return true
    end
    return false
end

-- ✅ icon command bus for this trigger
local function send_trigger_icon_cmd(targetId, cmd)
    SHARED.TRIGGER_ICON_CMD = SHARED.TRIGGER_ICON_CMD or {}
    SHARED.TRIGGER_ICON_CMD[targetId] = { cmd = cmd, by = targetId }
end

-- ---------------------------------------
-- Lifecycle
-- ---------------------------------------
function on_create(self)
    if DEBUG then
        print("[Trigger_MoveDown_And_Activate] on_create EntityID=" .. tostring(EntityID))
    end

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.TRIGGER_ICON_CMD = SHARED.TRIGGER_ICON_CMD or {}
end

function on_update(self, dt)
    dt = dt or 0.016

    local id = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    ----------------------------------------------------------------
    -- ✅ Drive icon (show only in range; hide forever after trigger)
    ----------------------------------------------------------------
    if not triggeredOnce then
        if in_range(id) then
            send_trigger_icon_cmd(id, "show")
        else
            send_trigger_icon_cmd(id, "hide")
        end
    else
        send_trigger_icon_cmd(id, "hide")
        return
    end

    ----------------------------------------------------------------
    -- Must be in interaction range
    ----------------------------------------------------------------
    if not in_range(id) then
        return
    end

    ----------------------------------------------------------------
    -- Must receive E-press command
    ----------------------------------------------------------------
    local ok = consume_shared_cmd(SHARED.DOOR_CMD, id)
    if not ok then
        return
    end

    ----------------------------------------------------------------
    -- TRIGGERED: perform actions
    ----------------------------------------------------------------
    move_self_y(id, MOVE_SELF_Y)

    if not set_scale(SCALE_ENTITY_ID, SCALE_TO[1], SCALE_TO[2], SCALE_TO[3]) then
        print("[Trigger_MoveDown_And_Activate] WARN: missing Transform on entity " .. tostring(SCALE_ENTITY_ID))
    end

    if not set_pos(MOVE_ENTITY_ID, MOVE_TO[1], MOVE_TO[2], MOVE_TO[3]) then
        print("[Trigger_MoveDown_And_Activate] WARN: missing Transform on entity " .. tostring(MOVE_ENTITY_ID))
    end

    -- Force icon hidden forever after trigger
    send_trigger_icon_cmd(id, "hide")

    triggeredOnce = true

    if DEBUG then
        print("[Trigger_MoveDown_And_Activate] TRIGGERED -> moved self to y="
            .. tostring(MOVE_SELF_Y)
            .. ", scaled " .. tostring(SCALE_ENTITY_ID)
            .. ", moved " .. tostring(MOVE_ENTITY_ID))
    end
end
