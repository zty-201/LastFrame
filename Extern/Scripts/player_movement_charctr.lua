-- ============================================================
-- player_movement_charctr.lua (Uses CharacterController Comp instead)
-- ============================================================

local moveSpeed = 100.0
local sprintMultiplier = 1.5
local jumpForce = 8.0
local deadZone = 0.1

local transform = nil
local camera = nil

local footstepDeadzone = 0.05
local pauseHeldPrev = false

-- Global Shared Helper
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

function on_create(self)
    print("[player_movement_charctr] on_create | EntityID=" .. tostring(EntityID))

    transform = EntityTransform
    camera    = GetMainCamera()

    -- API Check
    if type(Character_SetVelocity) ~= "function" then
        print("[player_movement_charctr][ERROR] CharacterController API missing (Character_SetVelocity).")
    end
    if type(Character_IsGrounded) ~= "function" then
        print("[player_movement_charctr][ERROR] CharacterController API missing (Character_IsGrounded).")
    end

    -- Footstep config/state
    footstepSlots = { "Footstep1","Footstep2","Footstep3","Footstep4","Footstep5","Footstep6","Footstep7","Footstep8" }
    lastFootstepIndex = 0
    footstepTimer = 0.0
    footstepIntervalMin = 0.6
    footstepIntervalMax = 1.20

    -- Seed RNG
    local seed = (tonumber(EntityID) or 0) * 1103515245 + 12345
    seed = seed % 2147483647
    math.randomseed(seed)

    _dbgPrintTimer = 0.0
end

function on_destroy(self)
    if type(Audio_StopSlot) == "function" then
        for i = 1, (footstepSlots and #footstepSlots or 0) do
            Audio_StopSlot(EntityID, footstepSlots[i])
        end
    end
    footstepTimer = 0.0
    lastFootstepIndex = 0
end

function on_update(self, dt)
    local state_name = StateMachine_GetCurrentStateName()
    if state_name == "Pause" then
        return
    end



    dt = dt or 0.016

    -- Heartbeat
    _dbgPrintTimer = (_dbgPrintTimer or 0.0) + dt
    if _dbgPrintTimer >= 1.0 then
        _dbgPrintTimer = _dbgPrintTimer - 1.0
        -- print(string.format("[player_movement_charctr] on_update heartbeat | dt=%.3f", dt))
    end

    local SHARED = getGlobalShared()
    local yaw = 0.0
    if SHARED and SHARED.POV then
        yaw = SHARED.POV.yaw or 0.0
    end

    -- ========================================================
    -- PAUSE INPUT
    -- ========================================================
    local pauseHeld = false
    if Input and Input.GetAction then
        pauseHeld = Input.GetAction("PAUSE") == true
    end
    
    local android_pause = SHARED.ANDROID_PAUSE_BUTTON

    if android_pause then
        pauseHeld = true
        android_pause = false
        SHARED.ANDROID_PAUSE_BUTTON = android_pause
    end

    local pausePressed = pauseHeld and not pauseHeldPrev
    pauseHeldPrev = pauseHeld

    if pausePressed then
        if UI_IsCutscenePlaying == nil or not UI_IsCutscenePlaying() then
            UI_CloseAllMenus()
            UI_DisplayMenu("PauseMenu")
            StateMachine_SetTrigger("PauseGame")
            print("========================================= PauseGame Trigger ============================================================")
            
        end
        return 
    end

    -- ========================================================
    -- MOVEMENT INPUT (Keyboard + UI Joystick)
    -- ========================================================
    local moveX, moveZ = 0.0, 0.0
    local joystickActive = false

    -- 1. Read from the C++ JoystickComponent via SHARED table
    if PLATFORM == "android" and SHARED and SHARED.UI then
        --UI_DisplayMenu("AndroidInputOverlay")
        
        local jx = -(SHARED.UI.js_X or 0.0)
        local jy = -(SHARED.UI.js_Y or 0.0)
        
        -- If joystick is moved beyond a tiny threshold, use it
        if math.abs(jx) > 0.01 or math.abs(jy) > 0.01 then
            moveX = jx
            moveZ = jy
            joystickActive = true
        end
    end

    --2. Read from Keyboard (Windows) - only if joystick isn't dominating
    if PLATFORM == "windows" then
        if Input.GetAction("MOVE_FORWARD") then moveZ = moveZ + 1 end
        if Input.GetAction("MOVE_BACK")    then moveZ = moveZ - 1 end
        if Input.GetAction("MOVE_LEFT")    then moveX = moveX + 1 end
        if Input.GetAction("MOVE_RIGHT")   then moveX = moveX - 1 end
    end

    local len = math.sqrt(moveX * moveX + moveZ * moveZ)

    -- ========================================================
    -- SPRINT LOGIC
    -- ========================================================
    local sprintHeld = false
    if Input and Input.GetAction then
        sprintHeld = Input.GetAction("SPRINT") == true
    end

    -- Auto-sprint if joystick is pushed to the outer edge (90%+)
    if joystickActive and len > 0.9 then
        sprintHeld = true
    end

    local isSprinting = sprintHeld and (len > deadZone)
    local speedMul = isSprinting and sprintMultiplier or 1.0

    -- ========================================================
    -- APPLY PHYSICS VELOCITY
    -- ========================================================
    if len > deadZone then
        -- Normalize if exceeding 1.0 (prevents diagonal speed boost)
        if len > 1.0 then
            moveX = moveX / len
            moveZ = moveZ / len
        end

        local radYaw = math.rad(yaw)
        local forwardX = math.sin(radYaw)
        local forwardZ = math.cos(radYaw)
        local rightX = math.cos(radYaw)
        local rightZ = -math.sin(radYaw)

        -- Relative to camera yaw
        local finalX = (rightX * moveX) + (forwardX * moveZ)
        local finalZ = (rightZ * moveX) + (forwardZ * moveZ)

        if type(Character_SetVelocity) == "function" then
            Character_SetVelocity(EntityID, finalX * moveSpeed * speedMul, 0, finalZ * moveSpeed * speedMul)
        end
    else
        if type(Character_SetVelocity) == "function" then
            Character_SetVelocity(EntityID, 0, 0, 0)
        end
    end

    -- ========================================================
    -- FOOTSTEPS
    -- ========================================================
    local inputMoving = (len > footstepDeadzone)

    local grounded = false
    if type(Character_IsGrounded) == "function" then
        grounded = Character_IsGrounded(EntityID)
    end

    _groundGrace = _groundGrace or 0.0
    local GRACE_SECONDS = 0.15 

    if grounded then
        _groundGrace = GRACE_SECONDS
    else
        _groundGrace = math.max(0.0, _groundGrace - dt)
    end

    local canStep = inputMoving and (_groundGrace > 0.0)

    if not inputMoving then
        footstepTimer = 0.0
        return
    end

    if not canStep then return end

    -- Interval scaling
    local t = len
    if t > 1.0 then t = 1.0 end
    
    local interval = footstepIntervalMax + (footstepIntervalMin - footstepIntervalMax) * t
    interval = interval / speedMul
    if interval < 0.15 then interval = 0.15 end

    footstepTimer = (footstepTimer or 0.0) + dt
    if footstepTimer >= interval then
        footstepTimer = footstepTimer - interval

        local count = #footstepSlots
        local idx = math.random(1, count)
        if idx == lastFootstepIndex and count > 1 then
            idx = (idx % count) + 1
        end
        lastFootstepIndex = idx

        local slotId = footstepSlots[idx]

        if type(Audio_StopSlot) == "function" then
            Audio_StopSlot(EntityID, slotId)
        end
        if type(Audio_PlaySlot) == "function" then
            Audio_PlaySlot(EntityID, slotId)
        end
    end
end
