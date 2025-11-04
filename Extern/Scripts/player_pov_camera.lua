-- ============================================================
-- player_touch_gyro_camera.lua
-- Bottom-left: joystick move | Gyroscope: look around
-- ============================================================

local moveSpeed       = 50.0
local deadZone        = 20.0
local yaw, pitch      = 0.0, 0.0
local gyroSensitivity = 10.0
local pitchLimit      = 89.0
local gyroEnabled     = true
local camera          = nil

-- Movement state
local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY     = 0.0, 0.0

-- Screen layout
local screenWidth     = 3088
local screenHeight    = 1440
local moveAreaRight   = screenWidth * 0.4
local moveAreaBottom  = screenHeight * 0.5

-- ============================================================
function on_create()
    print("[player_touch_gyro_camera.lua] Initialized (gyro first, move second)")
    camera = GetMainCamera()
    if not camera then
        print("[Lua] ERROR: No camera found!")
    else
        print("[Lua] Camera successfully linked.")
    end
end

-- ============================================================
function on_update(self, dt)
    if not camera then return end

    ------------------------------------------------------------
    --  GYROSCOPE LOOK (update orientation FIRST)
    ------------------------------------------------------------
    if gyroEnabled then
        local gyroX, gyroY, gyroZ = Input.GetGyroscopeRotatioRate()
        if math.abs(gyroX) > 0.001 or math.abs(gyroY) > 0.001 then
            local tempYaw   =  math.deg(gyroX) * dt * gyroSensitivity
            local tempPitch = -math.deg(gyroY) * dt * gyroSensitivity
            yaw   = yaw + tempYaw
            pitch = pitch + tempPitch
        end
    end

    -- Clamp pitch
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    -- Update camera orientation now
    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local forward = {
        x = cosPitch * math.sin(radYaw),
        y = math.sin(radPitch),
        z = cosPitch * math.cos(radYaw)
    }

    local camX, camY, camZ = camera:GetPosition()
    camera:LookAt(
        camX + forward.x,
        camY + forward.y,
        camZ + forward.z
    )

    ------------------------------------------------------------
    --  TOUCH MOVEMENT (uses *updated* yaw)
    ------------------------------------------------------------
    local touchDown = Input.GetMouseButtonDown(0)
    local touchHeld = Input.GetMouseButton(0)
    local touchUp   = Input.GetMouseButtonUp(0)
    local mx, my    = Input.GetMousePosition()

    if touchDown and mx < moveAreaRight and my > moveAreaBottom then
        moveTouchActive = true
        moveStartX, moveStartY = mx, my
        moveCurX, moveCurY = mx, my
    end

    if moveTouchActive and touchHeld then
        moveCurX, moveCurY = mx, my
        local dx = moveCurX - moveStartX
        local dy = moveCurY - moveStartY

        if math.abs(dx) < deadZone then dx = 0 end
        if math.abs(dy) < deadZone then dy = 0 end

        local moveX = dx / (screenWidth * 0.25)
        local moveZ = -dy / (screenHeight * 0.25)

        local len = math.sqrt(moveX^2 + moveZ^2)
        if len > 1.0 then
            moveX = moveX / len
            moveZ = moveZ / len
        end

        -- Use the *current* yaw (just updated from gyro)
        local forwardX = math.sin(radYaw)
        local forwardZ = math.cos(radYaw)
        local rightX   = math.sin(radYaw - math.pi / 2)
        local rightZ   = math.cos(radYaw - math.pi / 2)

        local worldMoveX = rightX * moveX + forwardX * moveZ
        local worldMoveZ = rightZ * moveX + forwardZ * moveZ

        local posX, posY, posZ = camera:GetPosition()
        posX = posX + worldMoveX * moveSpeed * dt
        posZ = posZ + worldMoveZ * moveSpeed * dt
        camera:SetPosition(posX, posY, posZ)
    end

    if touchUp then moveTouchActive = false end
end

-- ============================================================
function on_destroy()
    print("[player_touch_gyro_camera.lua] Destroyed")
end
