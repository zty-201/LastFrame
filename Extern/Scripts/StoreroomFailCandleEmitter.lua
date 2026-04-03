-- StoreroomFailCandleEmitter.lua
-- Attach to the separate fire emitter entity for each fail candle.
-- Controlled via SHARED.FAIL_CANDLE_EMITTER_CMD[EntityID]
--
-- Commands:
--   { cmd="off", by=<id> }   -> blow out this flame
--   { cmd="on",  by=<id> }   -> relight this flame (optional)
--   { cmd="set", by=<id>, emit=<bool> }

local DEBUG = false

local CMD_TABLE = "FAIL_CANDLE_EMITTER_CMD"

local EMITTER_ON_RATE  = 50.0
local EMITTER_OFF_RATE = 0.0

local currentRate = 0.0

local function ensure_shared()
    SHARED = SHARED or {}
    SHARED[CMD_TABLE] = SHARED[CMD_TABLE] or {}
end

local function get_emitter()
    local emitter = Query.GetParticleEmitter(EntityID)
    if emitter == nil then
        print("[StoreroomFailCandleEmitter] ERROR: No ParticleEmitterComponent on EntityID=" .. tostring(EntityID))
        return nil
    end
    return emitter
end

local function set_emitter_rate(rate)
    local emitter = get_emitter()
    if emitter == nil then
        return false
    end

    emitter.spawnRate = rate
    emitter.timeSinceLastSpawn = 0.0
    currentRate = rate
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

    if DEBUG then
        print(string.format(
            "[StoreroomFailCandleEmitter] Ready | Entity=%d | SpawnRate=%.2f",
            EntityID,
            currentRate
        ))
    end
end

function on_update(self, dt)
    ensure_shared()

    local cmdObj = SHARED[CMD_TABLE][EntityID]
    if not cmdObj then
        return
    end

    SHARED[CMD_TABLE][EntityID] = nil
    local cmd = cmdObj.cmd

    if cmd == "off" then
        set_emitter_rate(EMITTER_OFF_RATE)

        if DEBUG then
            print(string.format(
                "[StoreroomFailCandleEmitter] OFF | Entity=%d | by=%s",
                EntityID,
                tostring(cmdObj.by)
            ))
        end

    elseif cmd == "on" then
        set_emitter_rate(EMITTER_ON_RATE)

        if DEBUG then
            print(string.format(
                "[StoreroomFailCandleEmitter] ON | Entity=%d | by=%s",
                EntityID,
                tostring(cmdObj.by)
            ))
        end

    elseif cmd == "set" then
        local emit = cmdObj.emit
        if emit ~= nil then
            if emit == true then
                set_emitter_rate(EMITTER_ON_RATE)
            else
                set_emitter_rate(EMITTER_OFF_RATE)
            end
        end

        if DEBUG then
            print(string.format(
                "[StoreroomFailCandleEmitter] SET | Entity=%d | emit=%s | by=%s",
                EntityID,
                tostring(emit),
                tostring(cmdObj.by)
            ))
        end
    end
end