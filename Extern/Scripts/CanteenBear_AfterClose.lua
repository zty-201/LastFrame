print("[CanteenBear_AfterClose] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- Must match CanteenBearInterface.lua
local TOY_ID   = 376
local UI_TABLE = "CANTEEN_TOY_UI_OPEN"

-- Audio slot name (configure on this entity's Audio component)
local CRASH_AUDIO_SLOT = "Crash"

-- Entity IDs and their target transforms
local ENTITY_TRANSFORMS = {
    { id = 361, px = 217.767, py = 113.44,  pz = 657.093, rx = -9.65,   ry = 5.1,    rz = 4.8     },
    { id = 362, px = 235.382, py = 131.137, pz = 657.093, rx = 0.0,     ry = 0.0,    rz = 46.5    },
    { id = 363, px = 252.559, py = 143.031, pz = 664.160, rx = 0.0,     ry = 0.0,    rz = 35.650  },
    { id = 146, px = 264.023, py = 151.581, pz = 661.997, rx = 169.150, ry = 12.050, rz = -135.450},
}

local uiWasOpen = nil
local fired     = false

local function log(...)
    if DEBUG then print("[CanteenBear_AfterClose]", ...) end
end

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

local function do_crash()
    if fired then return end
    fired = true

    -- Play crashing sound
    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, CRASH_AUDIO_SLOT)
        log("Played audio slot:", CRASH_AUDIO_SLOT)
    else
        log("WARN: Audio_PlaySlot not available")
    end

    -- Trigger screen shake
    SHARED = SHARED or {}
    SHARED.POV = SHARED.POV or {}
    SHARED.POV.freeShake = true
    SHARED.POV.shakeDuration = 0.6
    SHARED.POV.shakeAmplitude = 4.5
    SHARED.POV.shakeSpeed = 0.03
    log("Triggered screen shake")

    -- Reposition and reorient entities
    for _, entry in ipairs(ENTITY_TRANSFORMS) do
        local tr = Query.GetTransform(entry.id)
        if tr then
            tr:SetPosition(entry.px, entry.py, entry.pz)
            tr:SetRotation(entry.rx, entry.ry, entry.rz)
            log(string.format("Entity %d -> pos(%.3f, %.3f, %.3f) rot(%.3f, %.3f, %.3f)",
                entry.id, entry.px, entry.py, entry.pz,
                entry.rx, entry.ry, entry.rz))
        else
            log("WARN: No transform found for entity", entry.id)
        end
    end

    -- Trigger entity 365 rolling sequence
    if SHARED.CanteenBearRoll and type(SHARED.CanteenBearRoll.Trigger) == "function" then
        SHARED.CanteenBearRoll.Trigger()
        log("Triggered CanteenBearRoll")
    else
        log("WARN: CanteenBearRoll.Trigger not available — is CanteenBear_RollEntity365.lua in the scene?")
    end
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    if SHARED[UI_TABLE][TOY_ID] == nil then
        SHARED[UI_TABLE][TOY_ID] = false
    end

    uiWasOpen = nil
    fired     = false

    log("Ready. EntityID:", EntityID)
end

function on_update(self, dt)
    local open = want_open()

    -- First frame: just record state, don't act
    if uiWasOpen == nil then
        uiWasOpen = open
        return
    end

    -- Detect close transition (open -> closed)
    if uiWasOpen == true and open == false then
        log("UI closed detected — triggering crash")
        do_crash()
    end

    uiWasOpen = open
end

function on_destroy(self)
end
