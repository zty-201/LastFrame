-- HallLightEmitter.lua
-- Attach to Particle Emitter entity
-- Controlled via SHARED.EMITTER_CMD[EntityID]
-- Commands:
--   { cmd="off", by=<id> }
--   { cmd="on",  by=<id> }
--   { cmd="set", by=<id>, emit=<bool> }

local EMITTER_ON_RATE  = 50.0
local EMITTER_OFF_RATE = 0.0
local IGNITION_SECONDS = 0.45

local currentRate = 0.0

local isIgniting = false
local ignitionElapsed = 0.0
local ignitionStartRate = 0.0
local ignitionTargetRate = EMITTER_ON_RATE

local function ensure_shared()
    SHARED = SHARED or {}
    SHARED.EMITTER_CMD = SHARED.EMITTER_CMD or {}
end

local function get_emitter()
    local emitter = Query.GetParticleEmitter(EntityID)
    if emitter == nil then
        print("[HallLightEmitter] ERROR: No ParticleEmitterComponent on EntityID=" .. tostring(EntityID))
        return nil
    end
    return emitter
end

local function apply_rate(rate)
    local emitter = get_emitter()
    if emitter == nil then
        return false
    end

    emitter.spawnRate = rate
    emitter.timeSinceLastSpawn = 0.0
    currentRate = rate
    return true
end

local function stop_ignition()
    isIgniting = false
    ignitionElapsed = 0.0
end

local function begin_ignition(targetRate)
    local emitter = get_emitter()
    if emitter == nil then
        return false
    end

    ignitionStartRate = 0.0
    ignitionTargetRate = targetRate or EMITTER_ON_RATE
    ignitionElapsed = 0.0
    isIgniting = true

    emitter.spawnRate = ignitionStartRate
    emitter.timeSinceLastSpawn = 0.0
    currentRate = ignitionStartRate
    return true
end

function on_create(self)
    ensure_shared()

    local emitter = get_emitter()
    if emitter == nil then
        return
    end

    currentRate = emitter.spawnRate
    emitter.timeSinceLastSpawn = 0.0
    stop_ignition()

    print(string.format(
        "[HallLightEmitter] ready EntityID=%d spawnRate=%.2f",
        EntityID,
        currentRate
    ))
end

function on_update(self, dt)
    ensure_shared()
    dt = dt or 0.016

    local emitter = get_emitter()
    if emitter == nil then
        return
    end

    local cmdObj = SHARED.EMITTER_CMD[EntityID]
    if cmdObj then
        SHARED.EMITTER_CMD[EntityID] = nil
        local cmd = cmdObj.cmd

        if cmd == "off" then
            stop_ignition()
            apply_rate(EMITTER_OFF_RATE)

        elseif cmd == "on" then
            begin_ignition(EMITTER_ON_RATE)

        elseif cmd == "set" then
            local emit = cmdObj.emit
            if emit ~= nil then
                if emit == true then
                    begin_ignition(EMITTER_ON_RATE)
                else
                    stop_ignition()
                    apply_rate(EMITTER_OFF_RATE)
                end
            end
        end

        print(string.format(
            "[HallLightEmitter] CMD=%s by=%s -> spawnRate=%.2f",
            tostring(cmd),
            tostring(cmdObj.by),
            currentRate
        ))
    end

    if isIgniting then
        ignitionElapsed = ignitionElapsed + dt

        local t = ignitionElapsed / IGNITION_SECONDS
        if t > 1.0 then t = 1.0 end

        local eased = t * t * (3.0 - 2.0 * t)
        local newRate = ignitionStartRate + (ignitionTargetRate - ignitionStartRate) * eased

        emitter.spawnRate = newRate
        emitter.timeSinceLastSpawn = 0.0
        currentRate = newRate

        if t >= 1.0 then
            emitter.spawnRate = ignitionTargetRate
            emitter.timeSinceLastSpawn = 0.0
            currentRate = ignitionTargetRate
            stop_ignition()
        end
    end
end