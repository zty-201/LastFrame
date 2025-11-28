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
    -- WINDOWS MOVEMENT (JOYSTICK-ONLY)
    -- ========================================================
    if PLATFORM == "windows" then   
        local moveX, moveZ = 0.0, 0.0

        if Input.GetKey(GLFW_KEY_W) then print("[MOVE] W pressed") moveZ = moveZ + 1 end
        if Input.GetKey(GLFW_KEY_S) then print("[MOVE] S pressed") moveZ = moveZ - 1 end
        if Input.GetKey(GLFW_KEY_A) then print("[MOVE] A pressed") moveX = moveX + 1 end
        if Input.GetKey(GLFW_KEY_D) then print("[MOVE] D pressed") moveX = moveX - 1 end

        -- Joystick input (from UI only)
        if SHARED and SHARED.UI then
            local jx = SHARED.UI.joystickX or 0.0
            local jy = SHARED.UI.joystickY or 0.0
            moveX = moveX + jx
            moveZ = moveZ + jy
        end

        local len = math.sqrt(moveX * moveX + moveZ * moveZ)
        if len > 0 then
            moveX, moveZ = moveX / len, moveZ / len

            local radYaw   = math.rad(yaw)
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
    -- ANDROID MOVEMENT USING UI JOYSTICK
    -- ========================================================
    if PLATFORM == "android" then
        local moveX, moveZ = 0.0, 0.0

        if SHARED and SHARED.UI then
            local jx = SHARED.UI.joystickX or 0.0
            local jy = SHARED.UI.joystickY or 0.0
            moveX = moveX + jx
            moveZ = moveZ + jy
        end

        local len = math.sqrt(moveX*moveX + moveZ*moveZ)
        if len > 0 then
            moveX, moveZ = moveX / len, moveZ / len

            local radYaw = math.rad(yaw)
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
end

-- ============================================================
function on_destroy()
    print("[player_movement] Destroyed for entity:", EntityID)
end