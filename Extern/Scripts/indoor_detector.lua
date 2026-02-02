-- indoor_audio_detector.lua
-- Attach to PLAYER entity.

print("[IndoorAudio] script file loaded")

local indoorZones = { 259 }   -- add more later: {259, 300, 301, ...}
local indoors = false

-- check rate (10 times/sec)
local timer = 0.0
local interval = 0.1

local function isInsideAnyZone(playerId)
    for i = 1, #indoorZones do
        if Physics_CheckCollision(playerId, indoorZones[i]) then
            return true
        end
    end
    return false
end

function on_create(self)
    print("[IndoorAudio] on_create, EntityID =", EntityID)
end

function on_update(self, dt)
    print("[IndoorAudio] inside=", tostring(Physics_CheckCollision(EntityID, 259)))
    timer = timer + dt
    if timer < interval then return end
    timer = 0.0

    local inside = isInsideAnyZone(EntityID)

    if inside ~= indoors then
        indoors = inside
        if indoors then
            print("[IndoorAudio] ENTER indoor zone")
            Audio_SetIndoorFactor(1.0)
        else
            print("[IndoorAudio] EXIT indoor zone")
            Audio_SetIndoorFactor(0.0)
        end
    end
end