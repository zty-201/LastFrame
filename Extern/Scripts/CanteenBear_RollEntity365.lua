print("[CanteenBear_RollEntity365] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local ROLL_ENTITY_ID = 365

-- Rolling parameters (tune these)
local ORBIT_RADIUS    = 60.0   -- radius of the circle in world units
local ORBIT_SPEED_DPS = 90.0   -- degrees per second for orbiting
local ROLL_SPIN_RATIO = 3.0    -- roll spins per orbit revolution
local TILT_ANGLE      = 15.0   -- inward lean while rolling (degrees)
local FLOOR_Y         = 22.0   -- Y position while rolling
local ROLL_DURATION   = 2.4    -- seconds of full-speed rolling before decel
local DECEL_TIME      = 1.0    -- seconds to decelerate to a full stop
local STOP_TIME       = 0.6    -- seconds to ease rotation smoothly back to 0

local STATE_IDLE     = 0
local STATE_ROLLING  = 1
local STATE_DECEL    = 2
local STATE_STOPPING = 3

local state      = STATE_IDLE
local orbitAngle = 0.0
local rollSpin   = 0.0   -- kept normalized [0, 360] every frame
local orbitSpeed = 0.0
local timer      = 0.0
local centerX    = 0.0
local centerZ    = 0.0

-- Snapshot of rotation when decel ends, normalized to [-180, 180] for lerping
local stopRotX  = 0.0
local stopRotY  = 0.0
local stopRotZ  = 0.0
local stopTimer = 0.0

local function log(...)
    if DEBUG then print("[CanteenBear_RollEntity365]", ...) end
end

local function deg2rad(d) return d * math.pi / 180.0 end

-- Wraps angle into [0, 360)
local function wrap360(a)
    a = a % 360.0
    if a < 0 then a = a + 360.0 end
    return a
end

-- Wraps angle into (-180, 180] — shortest distance to any target
local function normalize_angle(a)
    a = wrap360(a)
    if a > 180.0 then a = a - 360.0 end
    return a
end

-- Lerps from a to b along the shortest arc (handles wrap-around correctly)
local function lerp_angle(a, b, t)
    local diff = normalize_angle(b - a)
    return a + diff * t
end

local function lerp(a, b, t) return a + (b - a) * t end

local function trigger_roll()
    if state ~= STATE_IDLE then
        log("Already rolling — trigger ignored")
        return
    end

    local tr = Query.GetTransform(ROLL_ENTITY_ID)
    if not tr then
        log("WARN: no transform for entity", ROLL_ENTITY_ID)
        return
    end

    local sx, _, sz = tr:GetPosition()
    centerX = sx + ORBIT_RADIUS
    centerZ = sz

    orbitAngle = 180.0   -- starts directly west of center (= entity start pos)
    rollSpin   = 0.0
    orbitSpeed = ORBIT_SPEED_DPS
    timer      = 0.0
    state      = STATE_ROLLING

    log(string.format(
        "Roll triggered. Start=(%.2f, %.2f, %.2f)  Center=(%.2f, %.2f)",
        sx, FLOOR_Y, sz, centerX, centerZ))
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CanteenBearRoll = SHARED.CanteenBearRoll or {}
    SHARED.CanteenBearRoll._self = self

    function SHARED.CanteenBearRoll.Trigger()
        trigger_roll()
    end

    state = STATE_IDLE
    log("Ready. EntityID:", EntityID)
end

function on_update(self, dt)
    dt = dt or 0.016
    if state == STATE_IDLE then return end

    local tr = Query.GetTransform(ROLL_ENTITY_ID)
    if not tr then return end

    -- ── Full-speed rolling ─────────────────────────────────────────────────
    if state == STATE_ROLLING then
        timer      = timer + dt
        orbitAngle = wrap360(orbitAngle + orbitSpeed * dt)
        rollSpin   = wrap360(rollSpin   + orbitSpeed * ROLL_SPIN_RATIO * dt)

        local rad = deg2rad(orbitAngle)
        local px  = centerX + ORBIT_RADIUS * math.cos(rad)
        local pz  = centerZ + ORBIT_RADIUS * math.sin(rad)

        tr:SetPosition(px, FLOOR_Y, pz)
        tr:SetRotation(TILT_ANGLE, orbitAngle + 90.0, rollSpin)

        if timer >= ROLL_DURATION then
            state = STATE_DECEL
            timer = 0.0
            log("ROLLING -> DECEL")
        end

    -- ── Deceleration ───────────────────────────────────────────────────────
    elseif state == STATE_DECEL then
        timer = timer + dt
        local t = math.min(timer / DECEL_TIME, 1.0)

        orbitSpeed = ORBIT_SPEED_DPS * (1.0 - t)

        orbitAngle = wrap360(orbitAngle + orbitSpeed * dt)
        rollSpin   = wrap360(rollSpin   + orbitSpeed * ROLL_SPIN_RATIO * dt)

        local rad  = deg2rad(orbitAngle)
        local px   = centerX + ORBIT_RADIUS * math.cos(rad)
        local pz   = centerZ + ORBIT_RADIUS * math.sin(rad)
        local lean = TILT_ANGLE * (1.0 - t)

        tr:SetPosition(px, FLOOR_Y, pz)
        tr:SetRotation(lean, orbitAngle + 90.0, rollSpin)

        if timer >= DECEL_TIME then
            -- Normalize to [-180, 180] so lerp_angle always takes the short arc to 0
            stopRotX  = normalize_angle(lean)
            stopRotY  = normalize_angle(orbitAngle + 90.0)
            stopRotZ  = normalize_angle(rollSpin)
            stopTimer = 0.0
            state     = STATE_STOPPING
            log(string.format("DECEL -> STOPPING. Rot snapshot (%.2f, %.2f, %.2f)",
                stopRotX, stopRotY, stopRotZ))
        end

    -- ── Settle rotation back to 0 ─────────────────────────────────────────
    elseif state == STATE_STOPPING then
        stopTimer = stopTimer + dt
        local t   = math.min(stopTimer / STOP_TIME, 1.0)

        -- Smooth-step so it eases in and out naturally
        local ease = t * t * (3.0 - 2.0 * t)

        -- Shortest-arc lerp ensures no axis flips through the long way around
        local rx = lerp_angle(stopRotX, 0.0, ease)
        local ry = lerp_angle(stopRotY, 0.0, ease)
        local rz = lerp_angle(stopRotZ, 0.0, ease)

        tr:SetRotation(rx, ry, rz)

        if t >= 1.0 then
            tr:SetRotation(0.0, 0.0, 0.0)
            state = STATE_IDLE
            log("Entity", ROLL_ENTITY_ID, "settled. Done.")
        end
    end
end

function on_destroy(self)
    if SHARED.CanteenBearRoll and SHARED.CanteenBearRoll._self == self then
        SHARED.CanteenBearRoll._self = nil
    end
end
