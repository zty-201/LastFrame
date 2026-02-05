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
    print("[player_movement_v2] Created for Entity: " .. tostring(EntityID))

    transform = EntityTransform
    camera    = GetMainCamera()

    if transform then
        print("[player_movement] Transform OK")
    else
        print("[player_movement ERROR] Transform missing!!")
    end

    if camera then
        print("[player_movement] Camera OK")
    else
        print("[player_movement ERROR] Camera missing!!")
    end
    
    if Character_SetVelocity == nil then
        print("[ERROR] CharacterController API missing! Recompile Engine.")
    end
end

function on_destroy(self)
    if footstepPlaying then
        safeStopSlot("Footstep")
    end
end

function on_update(self, dt)
    local SHARED = getGlobalShared()

    local yaw = 0.0
    if SHARED and SHARED.POV then
        yaw = SHARED.POV.yaw or 0.0
    end

    local moveX, moveZ = 0.0, 0.0

    -- ========================================================
    -- WINDOWS MOVEMENT (WASD only)
    -- ========================================================
    if PLATFORM == "windows" then
        if Input.GetAction(Key_Actions.MOVE_FORWARD) then moveZ = moveZ + 1 end
        if Input.GetAction("MOVE_BACK") then moveZ = moveZ - 1 end
        if Input.GetAction("MOVE_LEFT")  then moveX = moveX + 1 end
        if Input.GetAction("MOVE_RIGHT") then moveX = moveX - 1 end
    end

    -- ========================================================
    -- ANDROID MOVEMENT USING UI JOYSTICK
    -- ========================================================
    if PLATFORM == "android" and SHARED and SHARED.UI then
        local jx = SHARED.UI.joystickX or 0.0
        local jy = SHARED.UI.joystickY or 0.0
        moveX = moveX + jx
        moveZ = moveZ + jy
    end

    local len = math.sqrt(moveX * moveX + moveZ * moveZ)

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

        Character_SetVelocity(EntityID, finalX * moveSpeed, 0 , finalZ * moveSpeed)
    else
        Character_SetVelocity(EntityID, 0, 0, 0)
    end

    local isMoving = len > footstepDeadzone
    local isGrounded = Character_IsGrounded(EntityID)

    if isMoving and isGrounded then
        if not footstepPlaying then
            safePlaySlot("Footstep")
            footstepPlaying = true
        end
    else
        if footstepPlaying then
            safeStopSlot("Footstep")
            footstepPlaying = false
        end
    end
end
