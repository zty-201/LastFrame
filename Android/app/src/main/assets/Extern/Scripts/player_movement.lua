-- ============================================================
-- player_movement.lua  (FIXED - Corrected direction vectors)
-- ============================================================

local moveSpeed = 50.0
local deadZone  = 20.0

local transform = nil
local camera    = nil

-- Android touch joystick state
local moveTouchActive = false
local moveStartX, moveStartY = 0.0, 0.0
local moveCurX, moveCurY = 0.0, 0.0

local screenWidth, screenHeight = 3088, 1440
local moveAreaRight  = screenWidth * 0.4
local moveAreaBottom = screenHeight * 0.5

local function getGlobalShared()
    return rawget(_G, "SHARED")
end

-- ============================================================
function on_create(self)
    print("[player_movement] on_create()")

    transform = EntityTransform
    camera = GetMainCamera()

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

    --[Ben] in order for my flashlight to be binded to the player pos
    local SHARED = getGlobalShared()
    if SHARED then
        SHARED.PLAYER = SHARED.PLAYER or {}
        SHARED.PLAYER.transform = transform
    end
end

-- ============================================================
local function movePlayer(worldMoveX, worldMoveZ, dt)
    if not transform then 
        print("[player_movement ERROR] movePlayer called but transform is NIL")
        return 
    end

    local x, y, z = transform:GetPosition()
    x = x + worldMoveX * moveSpeed * dt
    z = z + worldMoveZ * moveSpeed * dt

    transform:SetPosition(x, y, z)
    --if camera then camera:SetPosition(x, y, z) end (do not move cam in this script)

    --[Ben] in order for my flashlight to be binded to the player pos
    local SHARED = getGlobalShared()
    if SHARED and SHARED.PLAYER then
        SHARED.PLAYER.x = x
        SHARED.PLAYER.y = y
        SHARED.PLAYER.z = z
    end
end

-- ============================================================
function on_update(self, dt)
    if not transform then return end

    local SHARED = getGlobalShared()
    local yaw = 0.0
    if SHARED and SHARED.POV then
        yaw = SHARED.POV.yaw or 0.0
    end

    -- ========================================================
    -- WINDOWS MOVEMENT (FIXED)
    -- ========================================================
    if PLATFORM == "windows" then
        local moveX, moveZ = 0.0, 0.0

        if Input.GetKey(GLFW_KEY_W) then print("[MOVE] W pressed") moveZ = moveZ + 1 end
        if Input.GetKey(GLFW_KEY_S) then print("[MOVE] S pressed") moveZ = moveZ - 1 end
        if Input.GetKey(GLFW_KEY_A) then print("[MOVE] A pressed") moveX = moveX + 1 end
        if Input.GetKey(GLFW_KEY_D) then print("[MOVE] D pressed") moveX = moveX - 1 end

        local len = math.sqrt(moveX*moveX + moveZ*moveZ)
        if len > 0 then
            moveX, moveZ = moveX / len, moveZ / len

            local radYaw = math.rad(yaw)
            -- FIXED: Correct forward and right vector calculations
            local forwardX = math.sin(radYaw)
            local forwardZ = math.cos(radYaw)
            local rightX   = math.cos(radYaw)
            local rightZ   = -math.sin(radYaw)

            movePlayer(
                rightX * moveX + forwardX * moveZ,
                rightZ * moveX + forwardZ * moveZ,
                dt
            )
        end
        return
    end

    -- ========================================================
    -- ANDROID JOYSTICK MOVEMENT (FIXED)
    -- ========================================================
    if PLATFORM == "android" then
        local down = Input.GetMouseButtonDown(0)
        local held = Input.GetMouseButton(0)
        local up   = Input.GetMouseButtonUp(0)
        local mx, my = Input.GetMousePosition()

        -- Detect joystick activation
        if down and mx < moveAreaRight and my > moveAreaBottom then
            moveTouchActive = true
            moveStartX, moveStartY = mx, my
            moveCurX, moveCurY = mx, my

            print("[player_movement] Activated at (" .. mx .. ", " .. my .. ")")
        end

        if moveTouchActive and held then
            moveCurX, moveCurY = mx, my

            local dx = moveCurX - moveStartX
            local dy = moveCurY - moveStartY

            print(string.format("[player_movement] Raw dx=%0.2f dy=%0.2f", dx, dy))

            if math.abs(dx) < deadZone then dx = 0 end
            if math.abs(dy) < deadZone then dy = 0 end

            local moveX = dx / (screenWidth * 0.25)
            local moveZ = -dy / (screenHeight * 0.25)

            print(string.format("[player_movement] Normalized moveX=%0.3f moveZ=%0.3f", moveX, moveZ))

            local len = math.sqrt(moveX*moveX + moveZ*moveZ)
            if len > 1 then
                moveX = moveX / len
                moveZ = moveZ / len
            end

            local radYaw = math.rad(yaw)
            -- FIXED: Correct forward and right vector calculations
            local forwardX = math.sin(radYaw)
            local forwardZ = math.cos(radYaw)
            local rightX   = math.cos(radYaw)
            local rightZ   = -math.sin(radYaw)

            local worldMoveX = rightX * moveX + forwardX * moveZ
            local worldMoveZ = rightZ * moveX + forwardZ * moveZ

            print(string.format("[player_movement] WorldMoveX=%0.3f WorldMoveZ=%0.3f", worldMoveX, worldMoveZ))

            movePlayer(worldMoveX, worldMoveZ, dt)
        end

        if up then
            print("[JOYSTICK] Released")
            moveTouchActive = false
        end
    end
end

-- ============================================================
function on_destroy()
    print("[player_movement] Destroyed for entity:", EntityID)
end