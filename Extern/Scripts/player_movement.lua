-- ============================================================
-- player_movement.lua
-- Handles keyboard + joystick movement ONLY
-- Requires player_pov_camera.lua to be loaded first
-- ============================================================

local moveSpeed = 50.0
local deadZone  = 20.0

local transform = nil
local camera    = nil
local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY = 0.0, 0.0

local screenWidth, screenHeight = 3088, 1440
local moveAreaRight  = screenWidth * 0.4
local moveAreaBottom = screenHeight * 0.5

-- ============================================================
function on_create(self)
    transform = POV.transform()
    camera = GetMainCamera()

    if not transform then print("[MOVE ERROR] Transform missing") end
    if not camera then print("[MOVE ERROR] Camera missing") end

    print("[MOVE] Movement initialized")
end

-- ============================================================
local function movePlayer(worldMoveX, worldMoveZ, dt)
    local x, y, z = transform:GetPosition()

    x = x + worldMoveX * moveSpeed * dt
    z = z + worldMoveZ * moveSpeed * dt

    transform:SetPosition(x, y, z)
    camera:SetPosition(x, y, z)
end

-- ============================================================
function on_update(self, dt)
    if not transform or not camera then return end
    local yaw = POV.yaw()

    ------------------------------------------------------------
    -- WINDOWS MOVEMENT (WASD)
    ------------------------------------------------------------
    if PLATFORM == "windows" then
        local moveX, moveZ = 0.0, 0.0

        if Input.GetKey(87) then moveZ = moveZ + 1 end  -- W
        if Input.GetKey(83) then moveZ = moveZ - 1 end  -- S
        if Input.GetKey(65) then moveX = moveX - 1 end  -- A
        if Input.GetKey(68) then moveX = moveX + 1 end  -- D

        local len = math.sqrt(moveX*moveX + moveZ*moveZ)
        if len > 0 then
            moveX, moveZ = moveX/len, moveZ/len

            local radYaw = math.rad(yaw)
            local forwardX = math.sin(radYaw)
            local forwardZ = math.cos(radYaw)
            local rightX   = math.sin(radYaw - math.pi/2)
            local rightZ   = math.cos(radYaw - math.pi/2)

            movePlayer(
                rightX * moveX + forwardX * moveZ,
                rightZ * moveX + forwardZ * moveZ,
                dt
            )
        end
        return
    end

    ------------------------------------------------------------
    -- ANDROID JOYSTICK MOVEMENT
    ------------------------------------------------------------
    if PLATFORM == "android" then
        local down = Input.GetMouseButtonDown(0)
        local held = Input.GetMouseButton(0)
        local up   = Input.GetMouseButtonUp(0)
        local mx,my = Input.GetMousePosition()

        if down and mx < moveAreaRight and my > moveAreaBottom then
            moveTouchActive = true
            moveStartX, moveStartY = mx, my
            moveCurX, moveCurY = mx, my
        end

        if moveTouchActive and held then
            moveCurX, moveCurY = mx, my
            local dx = moveCurX - moveStartX
            local dy = moveCurY - moveStartY

            if math.abs(dx) < deadZone then dx = 0 end
            if math.abs(dy) < deadZone then dy = 0 end

            local moveX = dx / (screenWidth * 0.25)
            local moveZ = -dy / (screenHeight * 0.25)

            local len = math.sqrt(moveX*moveX + moveZ*moveZ)
            if len > 1 then
                moveX, moveZ = moveX/len, moveZ/len
            end

            local radYaw = math.rad(yaw)
            local forwardX = math.sin(radYaw)
            local forwardZ = math.cos(radYaw)
            local rightX   = math.sin(radYaw - math.pi/2)
            local rightZ   = math.cos(radYaw - math.pi/2)

            movePlayer(
                rightX * moveX + forwardX * moveZ,
                rightZ * moveX + forwardZ * moveZ,
                dt
            )
        end

        if up then moveTouchActive = false end
    end
end

-- ============================================================
function on_destroy()
    print("[MOVE] Movement destroyed")
end
