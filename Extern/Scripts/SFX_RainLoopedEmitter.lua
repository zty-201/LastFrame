----------------------------------------------------------------
-- SFX_RainLoopEmitter.lua
-- Plays a looping 3D rain sound forever when the scene starts.
----------------------------------------------------------------

print("[RainEmitter] Script loaded")

local DEBUG = true

-- Slot ID must match the audio slot in the inspector
local RAIN_SLOT_ID = "RainLoop"

local hasStarted = false

local function play_3d_loop(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

function on_create(self)

    if DEBUG then
        print("[RainEmitter] on_create EntityID =", EntityID)
    end

    -- Play rain once when the object is created
    if not hasStarted then
        play_3d_loop(RAIN_SLOT_ID)
        hasStarted = true

        if DEBUG then
            print("[RainEmitter] Rain loop started")
        end
    end
end

function on_update(self, dt)
    -- Nothing needed here
end

function on_destroy(self)
    -- Optional cleanup
    if DEBUG then
        print("[RainEmitter] destroyed")
    end
end