-- InteractionBox_InteractableOnly.lua
-- (Your previous version + minimal additions for icon triggering)

local PLAYER_ID = 5
local FORWARD_MODE = "camera"
local FORWARD_SOURCE_ID = PLAYER_ID

local OFFSET_FORWARD = 30.0
local OFFSET_Y = 0.8

local LOG_EVERY_N_FRAMES = 30

local inside = {}   -- [entityId] = true if inside trigger
local frame = 0

-- ✅ NEW: track last frame zone to hide when leaving
local prev_zone = {} -- [entityId] = true

local function get_player_transform()
    return Query.GetTransform(PLAYER_ID)
end

local function get_forward()
    if FORWARD_MODE == "camera" then
        return Query.GetCameraForward(FORWARD_SOURCE_ID)
    end
    return 0.0, 0.0, 1.0
end

local function is_interactable(id)
    return SHARED.INTERACTABLE ~= nil and SHARED.INTERACTABLE[id] == true
end

function GetInteractablesInZone()
    local list = {}
    for id, _ in pairs(inside) do
        if is_interactable(id) then
            list[#list + 1] = id
        end
    end
    table.sort(list, function(a, b) return tonumber(a) < tonumber(b) end)
    return list
end

function GetFirstInteractableInZone()
    local list = GetInteractablesInZone()
    if #list == 0 then return nil end
    return list[1]
end

local function print_interactables(prefix)
    prefix = prefix or "[InteractionBox]"
    local list = GetInteractablesInZone()
    if #list == 0 then
        print(prefix .. " Interactables: (none)")
        return
    end

    local s = ""
    for i = 1, #list do
        s = s .. tostring(list[i])
        if i < #list then s = s .. ", " end
    end
    print(prefix .. " Interactables IDs: " .. s)
end

-- ✅ NEW: icon command bus helper
local function send_icon_cmd(targetId, cmd)
    SHARED.ICON_CMD = SHARED.ICON_CMD or {}
    SHARED.ICON_CMD[targetId] = { cmd = cmd, by = PLAYER_ID }
end


local function getShared()
    return rawget(_G, "SHARED")
end
local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.UI = Shared.UI or {}
    return Shared.UI
end



function on_create(self)
    print("[InteractionBox] ready EntityID=" .. tostring(EntityID))

    SHARED.INTERACTABLE = SHARED.INTERACTABLE or {}
    SHARED.INTERACT_FN  = SHARED.INTERACT_FN  or {}

    SHARED.INTERACT_IN_ZONE = {}

    -- ✅ NEW
    SHARED.ICON_CMD = SHARED.ICON_CMD or {}
end

function on_update(self, dt)
    local pT = get_player_transform()
    if pT == nil or EntityTransform == nil then return end

    local px, py, pz = pT:GetPosition()
    local fx, fy, fz = get_forward()

    EntityTransform:SetPosition(
        px + fx * OFFSET_FORWARD,
        py + OFFSET_Y,
        pz + fz * OFFSET_FORWARD
    )

    -- publish ALL interactables currently in range
    local zone = {}
    for id, _ in pairs(inside) do
        if is_interactable(id) then
            zone[id] = true
        end
    end
    SHARED.INTERACT_IN_ZONE = zone

    ----------------------------------------------------------------
    -- ✅ NEW: trigger icon scripts (no key needed)
    -- Every frame: show for items in range
    -- When something leaves range: hide
    ----------------------------------------------------------------
    for id, _ in pairs(prev_zone) do
        if zone[id] ~= true then
            send_icon_cmd(id, "hide")
        end
    end

    for id, _ in pairs(zone) do
        send_icon_cmd(id, "show")
    end

    prev_zone = zone

    ----------------------------------------------------------------
    -- (KEEP) E press → interact (toggle door)
    ----------------------------------------------------------------
    
    local uiShared = ensureUIShared()
    if not uiShared then return end

    local interact = uiShared.actionRequested

    if interact  then

        interact = false
        uiShared.actionRequested = interact;

        local targetId = GetFirstInteractableInZone()

        if targetId ~= nil then
            SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
            SHARED.DOOR_CMD[targetId] = { cmd = "toggle", by = PLAYER_ID }
            print("[InteractionBox] E -> sent toggle to door " .. tostring(targetId))
        else
            print("[InteractionBox] E -> no interactable")
        end
    end

    -- periodic logging
    if LOG_EVERY_N_FRAMES and LOG_EVERY_N_FRAMES > 0 then
        frame = frame + 1
        if (frame % LOG_EVERY_N_FRAMES) == 0 then
            print_interactables("[InteractionBox][Periodic]")
        end
    end
end

function on_trigger_enter(self, otherId)
    if otherId == nil then return end

    local wasInside = inside[otherId] == true
    inside[otherId] = true

    if (not wasInside) and is_interactable(otherId) then
        print("[InteractionBox] ENTER interactable -> " .. tostring(otherId))
    end
end

function on_trigger_exit(self, otherId)
    if otherId == nil then return end
    inside[otherId] = nil
end

function on_trigger_stay(self, otherId)
    if otherId == nil then return end
    inside[otherId] = true
end
