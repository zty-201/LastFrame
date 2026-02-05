-- InteractionBox_InteractableOnly.lua
local PLAYER_ID = 5
local FORWARD_MODE = "camera"
local FORWARD_SOURCE_ID = PLAYER_ID

local OFFSET_FORWARD = 30.0
local OFFSET_Y = 0.8

local LOG_EVERY_N_FRAMES = 30

local inside = {}
local frame = 0

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

-- Returns ALL interactable IDs currently inside the box
function GetInteractablesInZone()
    local list = {}
    for id, _ in pairs(inside) do
        if is_interactable(id) then
            list[#list + 1] = id
        end
    end
    table.sort(list, function(a,b) return tonumber(a) < tonumber(b) end)
    return list
end

-- Returns ONE interactable ID (first after sorting), or nil
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
    for i=1,#list do
        s = s .. tostring(list[i])
        if i < #list then s = s .. ", " end
    end
    print(prefix .. " Interactables IDs: " .. s)
end

function on_create(self)
    print("[InteractionBox] ready EntityID=" .. tostring(EntityID))

    -- ✅ SAFE GUARD (does NOT change behaviour)
    SHARED.INTERACTABLE = SHARED.INTERACTABLE or {}
    SHARED.INTERACT_FN  = SHARED.INTERACT_FN  or {}
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

    ----------------------------------------------------------------
    -- 🔹 NEW: GLFW E press → interact
    ----------------------------------------------------------------
    if Input.GetKeyDown(GLFW_KEY_E) then
        local targetId = GetFirstInteractableInZone()

        if targetId ~= nil then
            SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
            SHARED.DOOR_CMD[targetId] = { cmd = "toggle", by = PLAYER_ID }
            print("[InteractionBox] E -> sent toggle to door " .. tostring(targetId))
        else
            print("[InteractionBox] E -> no interactable")
        end
    end

    ----------------------------------------------------------------

    if LOG_EVERY_N_FRAMES and LOG_EVERY_N_FRAMES > 0 then
        frame = frame + 1
        if (frame % LOG_EVERY_N_FRAMES) == 0 then
            print_interactables("[InteractionBox][Periodic]")
        end
    end
end

function on_trigger_enter(self, otherId)
    if otherId == nil then return end
    inside[otherId] = true

    if is_interactable(otherId) then
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
