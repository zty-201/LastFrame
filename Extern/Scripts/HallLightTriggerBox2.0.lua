-- TriggerBox_LightSwitch_Once.lua
-- Attach to trigger box entity
-- Turns off multiple lights ONCE when player enters (never turns back on)

local PLAYER_ID = 5

-- 👇 LIGHT IDS TO CONTROL
local LIGHT_IDS = {
    486,
    488
}

local triggered = false

local function send_light_cmd(cmd)
    SHARED.LIGHT_CMD = SHARED.LIGHT_CMD or {}

    for _, lightId in ipairs(LIGHT_IDS) do
        SHARED.LIGHT_CMD[lightId] = {
            cmd = cmd,
            by  = EntityID
        }
        print("[TriggerBox] sent '" .. cmd .. "' to light " .. tostring(lightId))
    end
end

function on_create(self)
    print("[TriggerBox] LightSwitch ONCE ready EntityID=" .. tostring(EntityID))
end

function on_trigger_enter(self, otherId)
    if otherId ~= PLAYER_ID then return end
    if triggered then return end

    triggered = true
    print("[TriggerBox] PLAYER ENTER (first time) -> lights OFF")

    send_light_cmd("off")
end

-- Intentionally do nothing on exit (so it never turns back on)
function on_trigger_exit(self, otherId)
    -- no-op
end
