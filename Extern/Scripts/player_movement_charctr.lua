-- ============================================================
-- player_movement_charctr.lua (Uses CharacterController Comp instead)
-- ============================================================

local moveSpeed = 100.0
local jumpForce = 8.0
local deadZone = 0.1

local transform = nil
local camera = nil

local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY = 0.0, 0.0

local screenWidth, screenHeight = 3088, 1440
local moveAreaRight = screenWidth * 0.4
local moveAreaBottom = screenHeight * 0.5

local footstepPlaying = false
local footstepDeadzone = 0.05

local pauseHeldPrev = false

local function getGlobalShared()
    return rawget(_G, "SHARED")
end

local function safePlaySlot(slotId)
    if type(Audio_PlaySlot) == "function" and type(slotId) == "string" then
        Audio_PlaySlot(EntityID, slotId)
    end
end

local function safeStopSlot(slotId)
    if type(Audio_StopSlot) == "function" and type(slotId) == "string" then
        Audio_StopSlot(EntityID, slotId)
    end
end

function on_create(self)
    print("[player_movement_charctr] on_create | EntityID=" .. tostring(EntityID))

    transform = EntityTransform
    camera    = GetMainCamera()

    print("[player_movement_charctr] Audio_PlaySlot type = " .. tostring(type(Audio_PlaySlot)))
    print("[player_movement_charctr] Audio_StopSlot type = " .. tostring(type(Audio_StopSlot)))
    print("[player_movement_charctr] Character_SetVelocity type = " .. tostring(type(Character_SetVelocity)))
    print("[player_movement_charctr] Character_IsGrounded type = " .. tostring(type(Character_IsGrounded)))

    -- If any of these are nil, your script will never work as expected
    if type(Character_SetVelocity) ~= "function" then
        print("[player_movement_charctr][ERROR] CharacterController API missing (Character_SetVelocity).")
    end
    if type(Character_IsGrounded) ~= "function" then
        print("[player_movement_charctr][ERROR] CharacterController API missing (Character_IsGrounded).")
    end

    -- Footstep config/state (make them locals that persist)
    footstepSlots = { "Footstep1","Footstep2","Footstep3","Footstep4","Footstep5","Footstep6","Footstep7","Footstep8" }
    lastFootstepIndex = 0
    footstepTimer = 0.0
    footstepIntervalMin = 0.6
    footstepIntervalMax = 1.20

    -- Seed RNG without os.time() (os may be sandboxed)
    local seed = (tonumber(EntityID) or 0) * 1103515245 + 12345
    seed = seed % 2147483647
    math.randomseed(seed)

    -- Debug heartbeat timer
    _dbgPrintTimer = 0.0

    print(string.format("[player_movement_charctr] Footstep slots=%d (expects EXACT IDs Footstep1..Footstep8)", #footstepSlots))
end


function on_destroy(self)
    -- stop anything still playing (safe even for one-shots)
    if type(Audio_StopSlot) == "function" then
        for i = 1, (footstepSlots and #footstepSlots or 0) do
            Audio_StopSlot(EntityID, footstepSlots[i])
        end
    end
    footstepTimer = 0.0
    lastFootstepIndex = 0
    print("[player_movement_charctr] on_destroy | EntityID=" .. tostring(EntityID))
end

function on_update(self, dt)
    dt = dt or 0.016

    -- Heartbeat: proves this script is the one running
    _dbgPrintTimer = (_dbgPrintTimer or 0.0) + dt
    if _dbgPrintTimer >= 1.0 then
        _dbgPrintTimer = _dbgPrintTimer - 1.0
        print(string.format("[player_movement_charctr] on_update heartbeat | dt=%.3f", dt))
    end

    local SHARED = getGlobalShared()
    local yaw = 0.0
    if SHARED and SHARED.POV then
        yaw = SHARED.POV.yaw or 0.0
    end

    local moveX, moveZ = 0.0, 0.0

    -- ========================================================
    -- PAUSE INPUT
    -- ========================================================

    local pauseHeld = false

    if Input and Input.GetAction then
        pauseHeld = Input.GetAction("PAUSE") == true
    end

    -- rising edge detection (pressed this frame)
    local pausePressed = pauseHeld and not pauseHeldPrev
    pauseHeldPrev = pauseHeld

    if pausePressed then
        print("[Player] Pause pressed -> opening PauseMenu")

        -- Optional safety check (if bound)
        if UI_IsCutscenePlaying == nil or not UI_IsCutscenePlaying() then
            UI_CloseAllMenus()
            UI_DisplayMenu("PauseMenu")
        end

        return -- stops movement this frame
    end

    -- ========================================================
    -- INPUT (this is what we'll use for footsteps too)
    -- ========================================================
    if PLATFORM == "windows" then
        if Input.GetAction(Key_Actions.MOVE_FORWARD) then moveZ = moveZ + 1 end
        if Input.GetAction("MOVE_BACK") then moveZ = moveZ - 1 end
        if Input.GetAction("MOVE_LEFT")  then moveX = moveX + 1 end
        if Input.GetAction("MOVE_RIGHT") then moveX = moveX - 1 end
    end

    if PLATFORM == "android" and SHARED and SHARED.UI then
        local jx = SHARED.UI.joystickX or 0.0
        local jy = SHARED.UI.joystickY or 0.0
        moveX = moveX + jx
        moveZ = moveZ + jy
    end

    local len = math.sqrt(moveX * moveX + moveZ * moveZ)

    -- ========================================================
    -- MOVEMENT
    -- ========================================================
    if len > deadZone then
        if len > 1.0 then
            moveX = moveX / len
            moveZ = moveZ / len
        end

        local radYaw = math.rad(yaw)
        local forwardX = math.sin(radYaw)
        local forwardZ = math.cos(radYaw)
        local rightX = math.cos(radYaw)
        local rightZ = -math.sin(radYaw)

        local finalX = (rightX * moveX) + (forwardX * moveZ)
        local finalZ = (rightZ * moveX) + (forwardZ * moveZ)

        if type(Character_SetVelocity) == "function" then
            Character_SetVelocity(EntityID, finalX * moveSpeed, 0, finalZ * moveSpeed)
        end
    else
        if type(Character_SetVelocity) == "function" then
            Character_SetVelocity(EntityID, 0, 0, 0)
        end
    end

    -- ========================================================
    -- FOOTSTEPS (based on INPUT HELD, not actual movement)
    -- ========================================================
    local inputMoving = (len > footstepDeadzone)

    -- Grounded check can flicker in tight spaces, so give it a grace window
    local grounded = false
    if type(Character_IsGrounded) == "function" then
        grounded = Character_IsGrounded(EntityID)
    end

    _groundGrace = _groundGrace or 0.0
    local GRACE_SECONDS = 0.15 -- keeps footsteps going through small grounding glitches

    if grounded then
        _groundGrace = GRACE_SECONDS
    else
        _groundGrace = math.max(0.0, _groundGrace - dt)
    end

    local canStep = inputMoving and (_groundGrace > 0.0)

    -- If player stops pressing movement, reset so it doesn't "catch up" with delayed steps
    if not inputMoving then
        footstepTimer = 0.0
        return
    end

    -- If input is held but we're not allowed to step (truly airborne), pause timer
    if not canStep then
        -- optional: small debug once in a while
        if _dbgPrintTimer < 0.05 then
            print(string.format("[Footstep] paused (air) | inputMoving=%s len=%.2f grounded=%s grace=%.2f",
                tostring(inputMoving), len, tostring(grounded), _groundGrace
            ))
        end
        return
    end

    -- Interval scaling: stronger input -> faster steps (WASD ~1, joystick 0..1)
    local t = len
    if t > 1.0 then t = 1.0 end
    if t < 0.0 then t = 0.0 end

    local interval = footstepIntervalMax + (footstepIntervalMin - footstepIntervalMax) * t

    footstepTimer = (footstepTimer or 0.0) + dt
    if footstepTimer >= interval then
        footstepTimer = footstepTimer - interval

        local count = (footstepSlots and #footstepSlots) or 0
        if count <= 0 then
            print("[Footstep][ERROR] footstepSlots empty (check on_create ran)")
            return
        end

        local idx = math.random(1, count)
        if idx == lastFootstepIndex and count > 1 then
            idx = (idx % count) + 1
        end
        lastFootstepIndex = idx

        local slotId = footstepSlots[idx]

        print(string.format("[Footstep] PLAY %s | inputLen=%.2f interval=%.2f grounded=%s grace=%.2f",
            slotId, len, interval, tostring(grounded), _groundGrace
        ))

        -- Force retrigger one-shot
        if type(Audio_StopSlot) == "function" then
            Audio_StopSlot(EntityID, slotId)
        end
        if type(Audio_PlaySlot) == "function" then
            Audio_PlaySlot(EntityID, slotId)
        end
    end
end
