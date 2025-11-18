-- ============================================================
-- player_touch_gyro_camera.lua
-- Android: joystick move + gyro look
-- Windows: WASD move + mouse look
-- ============================================================

local moveSpeed       = 50.0
local deadZone        = 20.0
local yaw, pitch      = 0.0, 0.0
local gyroSensitivity = 5.0
local mouseSensitivity = 0.2
local pitchLimit      = 89.0
local gyroEnabled     = true
local camera          = nil

-- Movement state (for Android)
local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY     = 0.0, 0.0

-- Screen layout (fallback default, s23 ultra)
local screenWidth, screenHeight = 3088, 1440
-- if Input.GetScreenSize then
--     screenWidth, screenHeight = Input.GetScreenSize()
-- end
local moveAreaRight   = screenWidth * 0.4
local moveAreaBottom  = screenHeight * 0.5

-- ============================================================
function on_create()
    print(string.format("[player_pov_camera.lua] Initialized (%s)", PLATFORM or "unknown"))
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

    -- ============================================================
    -- WINDOWS CONTROLS (WASD + Mouse)
    -- ============================================================
    if PLATFORM == "windows" then
        -- Mouse look
        local dx, dy = Input.GetMouseDelta()
        yaw   = yaw + dx * mouseSensitivity
        pitch = pitch - dy * mouseSensitivity
        if pitch > pitchLimit then pitch = pitchLimit end
        if pitch < -pitchLimit then pitch = -pitchLimit end

        local radYaw   = math.rad(yaw)
        local radPitch = math.rad(pitch)
        local cosPitch = math.cos(radPitch)

        local forward = {
            x = cosPitch * math.sin(radYaw),
            y = math.sin(radPitch),
            z = cosPitch * math.cos(radYaw)
        }

        local camX, camY, camZ = camera:GetPosition()
        camera:LookAt(camX + forward.x, camY + forward.y, camZ + forward.z)

        -- WASD movement
        local moveX, moveZ = 0.0, 0.0
        if Input.GetKey("W") then moveZ = moveZ + 1 end
        if Input.GetKey("S") then moveZ = moveZ - 1 end
        if Input.GetKey("A") then moveX = moveX - 1 end
        if Input.GetKey("D") then moveX = moveX + 1 end

        local len = math.sqrt(moveX^2 + moveZ^2)
        if len > 0 then
            moveX, moveZ = moveX / len, moveZ / len
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

        return
    end

    -- ============================================================
    -- ANDROID CONTROLS (Joystick + Gyroscope)
    -- ============================================================
    if PLATFORM == "android" then
        -- ---------------------------
        -- GYROSCOPE LOOK
        -- ---------------------------
        if gyroEnabled then
            local gyroX, gyroY, gyroZ = Input.GetGyroscopeRotatioRate()
            if math.abs(gyroX) > 0.001 or math.abs(gyroY) > 0.001 then
                local tempYaw   =  math.deg(gyroX) * dt * gyroSensitivity
                local tempPitch = -math.deg(gyroY) * dt * gyroSensitivity
                yaw   = yaw + tempYaw
                pitch = pitch + tempPitch
            end
        end
        if pitch > pitchLimit then pitch = pitchLimit end
        if pitch < -pitchLimit then pitch = -pitchLimit end

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

        -- ---------------------------
        -- TOUCH MOVEMENT (Joystick)
        -- ---------------------------
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
            if len > 1.0 then moveX, moveZ = moveX / len, moveZ / len end

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
end

-- ============================================================
function on_destroy()
    print("[player_pov_camera.lua] Destroyed")
end
