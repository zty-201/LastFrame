----------------------------------------------------------------
-- clean_footsteps.lua
-- Guaranteed: only one loop ever plays. No duplicates.
----------------------------------------------------------------

local FADE_SPEED  = 3.0
local TARGET_VOL  = 0.55

local currentVolume = 0.0
local isPlaying = false
local isWalking = false

function on_create(self)
    if not EntityAudio then
        print("[Footsteps] ERROR: No AudioComponent!")
        return
    end

    EntityAudio:SetLoop(true)
    EntityAudio:SetVolume(0.0)
end

local function IsMoving()
    return  Input.GetKey(GLFW_KEY_W)
        or Input.GetKey(GLFW_KEY_A)
        or Input.GetKey(GLFW_KEY_S)
        or Input.GetKey(GLFW_KEY_D)
end

function on_update(self, dt)
    if not EntityAudio then return end

    ------------------------------------------------------------
    -- Detect walking state
    ------------------------------------------------------------
    local moving = IsMoving()

    ------------------------------------------------------------
    -- Start playing (ONLY ONCE)
    ------------------------------------------------------------
    if moving and not isPlaying then
        EntityAudio:RequestPlay()   -- create 1 loop
        isPlaying = true
        -- print("[Footsteps] START LOOP")
    end

    ------------------------------------------------------------
    -- Fade logic
    ------------------------------------------------------------
    if moving then
        -- Fade IN
        currentVolume = currentVolume + FADE_SPEED * dt
        if currentVolume > TARGET_VOL then
            currentVolume = TARGET_VOL
        end
    else
        -- Fade OUT
        currentVolume = currentVolume - FADE_SPEED * dt
        if currentVolume < 0 then currentVolume = 0 end

        -- Once volume is zero, we consider the loop “silent”
        if currentVolume <= 0.01 then
            isPlaying = false   -- allow next play
            -- (never call RequestPlay here)
        end
    end

    ------------------------------------------------------------
    -- Apply volume to active loop
    ------------------------------------------------------------
    EntityAudio:SetVolume(currentVolume)
end
