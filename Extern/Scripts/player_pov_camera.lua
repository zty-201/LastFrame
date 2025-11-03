-- ============================================================
-- player_touch_camera.lua
-- Mobile-friendly player POV camera
-- Bottom-left: movement | Left mouse / touch drag: look around
-- Movement now relative to facing direction (yaw)
-- ============================================================

local sensitivity = 0.25     -- how fast camera rotates
local moveSpeed   = 100.0      -- how fast camera moves
local deadZone    = 20.0     -- ignore small touch drags
local yaw, pitch  = 0.0, 0.0
local gyroSensitivity = 15.0
local pitchLimit = 89.0

local gyroEnabled = true

local camera = nil

-- Track states
local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY     = 0.0, 0.0

local rotating = false
local lastX, lastY = 0.0, 0.0

-- Adjust these for your screen layout
local screenWidth  = 1920
local screenHeight = 1080
local moveAreaRight = screenWidth * 0.4   -- left 40% for movement
local moveAreaBottom = screenHeight * 0.5 -- bottom half of screen

function on_create()
    print("[player_touch_camera.lua] Initialized (bottom-left move, LMB look)")
    camera = GetMainCamera()
    if not camera then
        print("[Lua] ERROR: No camera found!")
    else
        print("[Lua] Camera successfully linked.")
    end
end

function on_update(self, dt)
    if not camera then return end

    local deltaYaw = 0.0
    local deltaPitch = 0.0
    -- =======================================
    -- TOUCH AREA: MOVEMENT (bottom-left)
    -- =======================================
    local touchDown = Input.GetMouseButtonDown(0)
    local touchHeld = Input.GetMouseButton(0)
    local touchUp   = Input.GetMouseButtonUp(0)
    local mx, my    = Input.GetMousePosition()

    -- Start movement if in bottom-left area
    if touchDown and mx < moveAreaRight and my > moveAreaBottom then
        moveTouchActive = true
        moveStartX, moveStartY = mx, my
        moveCurX, moveCurY = mx, my
        --print("[Lua] Movement touch start")
    end

    -- Continue movement
    if moveTouchActive and touchHeld then
        moveCurX, moveCurY = mx, my
        local dx = moveCurX - moveStartX
        local dy = moveCurY - moveStartY

        if math.abs(dx) < deadZone then dx = 0 end
        if math.abs(dy) < deadZone then dy = 0 end

        -- Normalized screen drag direction
        local moveX = dx / (screenWidth * 0.25)
        local moveZ = -dy / (screenHeight * 0.25)

        local len = math.sqrt(moveX^2 + moveZ^2)
        if len > 1.0 then
            moveX = moveX / len
            moveZ = moveZ / len
        end

        -- Calculate camera-relative forward and right
        local radYaw = math.rad(yaw)
        local forwardX = math.cos(radYaw)
        local forwardZ = math.sin(radYaw)
        local rightX   = math.cos(radYaw + math.pi / 2)
        local rightZ   = math.sin(radYaw + math.pi / 2)

        -- Combine movement vectors based on facing direction
        local worldMoveX = (-rightX * moveX + forwardX * moveZ)
        local worldMoveZ = (-rightZ * moveX + forwardZ * moveZ)

        -- Apply movement
        local posX, posY, posZ = camera:GetPosition()
        posX = posX + worldMoveX * moveSpeed * dt
        posZ = posZ + worldMoveZ * moveSpeed * dt
        camera:SetPosition(posX, posY, posZ)
    end

    -- End movement
    if touchUp then
        moveTouchActive = false
    end

    -- =======================================
    -- LOOK ROTATION (LMB / touch drag)
    -- =======================================
    local lookDown = Input.GetMouseButtonDown(0)
    local lookHeld = Input.GetMouseButton(0)
    local lookUp   = Input.GetMouseButtonUp(0)

    -- only start rotating if NOT in movement zone
    if lookDown and not (mx < moveAreaRight and my > moveAreaBottom) then
        rotating = true
        lastX, lastY = mx, my
    elseif lookUp then
        rotating = false
    end

    if rotating and lookHeld then
        local dx = mx - lastX
        local dy = my - lastY

        -- 🌟 Accumulate into local deltas, do NOT update global yaw/pitch 🌟
        deltaYaw = deltaYaw + dx * sensitivity
        deltaPitch = deltaPitch - dy * sensitivity
        
        lastX, lastY = mx, my 
        -- Clamping and final application will now happen in the UNIFIED block
    end

    -- =======================================
    -- GYROSCOPE LOOK CONTROL
    -- =======================================
    if gyroEnabled and not moveTouchActive then 
        local gyroX, gyroY, gyroZ = Input.GetGyroscopeRotatioRate()
        local dt = dt 
        
        -- Skip if gyro is reporting zero (or near-zero) rate
        if math.abs(gyroX) > 0.001 or math.abs(gyroY) > 0.001 then
        
            -- Calculate intermediate deltas
            local tempYaw = -math.deg(gyroX) * dt * gyroSensitivity 
            local tempPitch = math.deg(gyroY) * dt * gyroSensitivity
        
            -- Accumulate gyro change
            deltaYaw = deltaYaw + tempYaw
            deltaPitch = deltaPitch + tempPitch
        end
    end

-- =======================================
    -- UPDATE CAMERA ORIENTATION (UNIFIED)
    -- =======================================
    -- Apply the rotation only if an accumulated delta exists
    if math.abs(deltaYaw) > 0 or math.abs(deltaPitch) > 0 then 
        
        -- Apply accumulated local change to the GLOBAL variables
        yaw = yaw + deltaYaw
        pitch = pitch + deltaPitch
        
        -- Clamp pitch
        if pitch > pitchLimit then pitch = pitchLimit end
        if pitch < -pitchLimit then pitch = -pitchLimit end
        
        -- Calculate the new 'Front' vector using yaw and pitch
        local radYaw   = math.rad(yaw)
        local radPitch = math.rad(pitch)
        local cosPitch = math.cos(radPitch)
        
        local forward = {
            x = cosPitch * math.sin(radYaw), -- Note: sin/cos mapping is often swapped depending on engine conventions
            y = math.sin(radPitch),
            z = cosPitch * math.cos(radYaw)
        }

        local camX, camY, camZ = camera:GetPosition()
        local lookAt = {
            x = camX + forward.x,
            y = camY + forward.y,
            z = camZ + forward.z
        }
        camera:LookAt(lookAt.x, lookAt.y, lookAt.z)
    end
end

-- function ToggleGyro(enable)
--     Check if 'enable' is explicitly provided; otherwise, toggle the current state
--     if enable ~= nil then
--         gyroEnabled = enable
--     else
--         gyroEnabled = not gyroEnabled
--     end
--     print("[Lua] Gyroscope Toggled: " .. tostring(gyroEnabled))
-- end

function on_destroy()
    print("[player_touch_camera.lua] Destroyed")
end
