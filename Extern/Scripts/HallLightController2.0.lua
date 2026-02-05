-- Light_SharedControlled.lua
-- Attach to Light entity
-- Controlled via SHARED.LIGHT_CMD[EntityID]
-- Commands:
--   { cmd="off", by=<id> }
--   { cmd="on",  by=<id> }
--   { cmd="set", by=<id>, value=<number> }

local defaultIntensity = 1.0
local currentIntensity = 1.0

local function ensure_shared()
    SHARED.LIGHT_CMD = SHARED.LIGHT_CMD or {}
end

function on_create(self)
    ensure_shared()

    if EntityLight == nil then
        print("[Light] ERROR: No LightComponent on EntityID=" .. tostring(EntityID))
        return
    end

    -- capture initial intensity as "default on" intensity
    currentIntensity = EntityLight:GetIntensity()
    defaultIntensity = currentIntensity

    print(string.format(
        "[Light] ready EntityID=%d defaultIntensity=%.2f",
        EntityID,
        defaultIntensity
    ))
end

function on_update(self, dt)
    if EntityLight == nil then return end
    if SHARED.LIGHT_CMD == nil then return end

    local cmdObj = SHARED.LIGHT_CMD[EntityID]
    if not cmdObj then return end

    -- consume command (IMPORTANT)
    SHARED.LIGHT_CMD[EntityID] = nil

    local cmd = cmdObj.cmd

    if cmd == "off" then
        currentIntensity = 0.0

    elseif cmd == "on" then
        currentIntensity = defaultIntensity

    elseif cmd == "set" then
        local v = cmdObj.value
        if v ~= nil then
            currentIntensity = tonumber(v) or currentIntensity
        end
    end

    -- apply
    EntityLight:SetIntensity(currentIntensity)

    print(string.format(
        "[Light] CMD=%s by=%s -> intensity=%.2f",
        tostring(cmd),
        tostring(cmdObj.by),
        currentIntensity
    ))
end
