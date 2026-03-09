-- ============================================================
-- player_pov_camera.lua
-- Handles FPS Look (mouse/gyro) only
-- MUST BE LOADED FIRST (before player_movement.lua)
-- ============================================================

local mouseSensitivity = 0.08
local gyroSensitivity  = 5.0
local pitchLimit       = 89.0
local gyroEnabled      = true

local camera      = nil
local transform   = nil
local yaw, pitch  = 0.0, 0.0
local lensMenuName = "DirtyLens"
local lensVisible  = false

-- Recenter smoothing
local recenterActive        = false
local recenterTime          = 0.0
local recenterDuration      = 0.25   -- seconds for smooth return
local recenterStartYaw      = 0.0
local recenterStartPitch    = 0.0
local recenterTargetYaw     = 0.0    -- where "centre" faces
local recenterTargetPitch   = 0.0

-- FPS mouse look (desktop)
local holdToLook   = true
local lookButton   = GLFW_MOUSE_BUTTON_RIGHT
local prevLookHeld = false

-- ============================================================
-- Access true global table
-- ============================================================
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

local function setMouseLookActive(active)
    Input.SetCursorLocked(active)       -- FPS: lock + unlimited deltas
    Input.SetCursorVisible(not active)  -- hide cursor while locked
end

local function setLensOverlayVisible(active)
    if active then
        if not lensVisible and UI_DisplayMenu ~= nil then
            UI_DisplayMenu(lensMenuName)
            lensVisible = true
        end
    else
        if lensVisible then
            if UI_CloseMenu ~= nil then
                UI_CloseMenu(lensMenuName)
            elseif UI_CloseAllMenus ~= nil then
                UI_CloseAllMenus()
            end
            lensVisible = false
        end
    end
end

local function updateLensOverlayVisibility()
    local cutscene = false
    if UI_IsCutscenePlaying ~= nil then
        cutscene = UI_IsCutscenePlaying()
    end

    local startMenu = false
    if UI_IsMenuActive ~= nil then
        startMenu = UI_IsMenuActive("StartMenu")
    end

    setLensOverlayVisible(not cutscene and not startMenu)
end

-- ============================================================
function on_create(self)
    transform = EntityTransform
    camera = GetMainCamera()

    if not transform then 
        print("[player_pov_camera ERROR] Transform missing")
    end
    if not camera then 
        print("[player_pov_camera ERROR] Camera missing")
    end

    pitch = 0.0
    yaw   = 0.0

    -- Setup SHARED.POV
    local SHARED = rawget(_G, "SHARED")
    if not SHARED then
        print("[player_pov_camera ERROR] Missing SHARED global!")
        return
    end

    if not SHARED.POV then
        SHARED.POV = {}
        print("[player_pov_camera] Created SHARED.POV")
    end

    SHARED.POV.yaw   = yaw
    SHARED.POV.pitch = pitch
    SHARED.POV.cam   = camera   -- <<<<<< THE FIX

    print("[player_pov_camera] Initialized. yaw=", yaw, " pitch=", pitch)
    updateLensOverlayVisibility()
end



-- ============================================================
-- Apply rotation + smoothing
-- ============================================================
local function applyLook()
    -- Clamp pitch
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    -- Make player model face where camera is yawing (keep existing pitch/roll)
    --local rx, ry, rz = transform:GetRotation()

    -- Common convention: yaw around Y axis
    -- If your model faces the wrong direction, add +/-90 or 180 offset.
    --transform:SetRotation(rx, yaw, rz)

    -- Player position
    --local x, y, z = transform:GetPosition()

    if Transform_SetRotation then
        Transform_SetRotation(EntityID, 0.0, yaw, 0.0)
    else
        transform:SetRotation(0.0, yaw, 0.0)
    end

    -- Compute forward vector
    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    -- -------------------------------
    -- Smoothed camera follow
    -- -------------------------------
    local x, y, z = transform:GetPosition()
    local smooth = 1.0

    local cx, cy, cz = camera:GetPosition()
    local nx = cx + (x - cx) * smooth
    local ny = cy + (y - cy) * smooth
    local nz = cz + (z - cz) * smooth

    camera:SetPosition(nx, ny, nz)
    camera:LookAt(nx + fx, ny + fy, nz + fz)

    -- <<<<<< UPDATE SHARED.POV EVERY FRAME
    local SHARED = rawget(_G, "SHARED")
    if SHARED and SHARED.POV then
        SHARED.POV.yaw   = yaw
        SHARED.POV.pitch = pitch
        SHARED.POV.cam   = camera   -- <<<<<< REQUIRED for raycast
    end
end

function on_update(self, dt)
    if not camera or not transform then return end

    updateLensOverlayVisibility()

    local SHARED = getGlobalShared()

    -- ======================================
    -- HANDLE RECENTER REQUEST FROM UI
    -- ======================================
    if SHARED and SHARED.UI and SHARED.UI.recenterRequested then
        -- Start a smooth recenter animation from current yaw/pitch
        recenterActive      = true
        recenterTime        = 0.0
        recenterStartYaw    = yaw
        recenterStartPitch  = pitch
        recenterTargetYaw   = 0.0   -- change these if your "centre" is different
        recenterTargetPitch = 0.0

        SHARED.UI.recenterRequested = false
    end

    -- ======================================
    -- WINDOWS LOOK CONTROL
    -- ======================================
    if PLATFORM == "windows" then

        if recenterActive then
            -- Progress smoothing
            recenterTime = recenterTime + dt
            local t = recenterTime / recenterDuration
            if t >= 1.0 then
                t = 1.0
                recenterActive = false
            end

            -- Smoothstep (nicer than linear)
            local s = t * t * (3.0 - 2.0 * t)

            yaw   = recenterStartYaw   + (recenterTargetYaw   - recenterStartYaw)   * s
            pitch = recenterStartPitch + (recenterTargetPitch - recenterStartPitch) * s

            applyLook()
            return
        end

        -- Normal mouse look when not recentering
        
        -- FPS-style mouse look (hold RMB to look)
        local lookHeld = true
        if holdToLook then
            lookHeld = Input.GetMouseButton(lookButton)
        end

        -- Toggle cursor lock/visibility when state changes
        if lookHeld ~= prevLookHeld then
            setMouseLookActive(lookHeld)
            prevLookHeld = lookHeld
        end

        -- Apply look while locked
        if lookHeld then
            local dx, dy = Input.GetMouseDelta()
            yaw   = yaw + dx * mouseSensitivity
            pitch = pitch - dy * mouseSensitivity
        end


        applyLook()
        return
    end

    -- ======================================
    -- ANDROID GYRO LOOK
    -- ======================================
    if PLATFORM == "android" and gyroEnabled then

        if recenterActive then
            recenterTime = recenterTime + dt
            local t = recenterTime / recenterDuration
            if t >= 1.0 then
                t = 1.0
                recenterActive = false
            end

            local s = t * t * (3.0 - 2.0 * t)

            yaw   = recenterStartYaw   + (recenterTargetYaw   - recenterStartYaw)   * s
            pitch = recenterStartPitch + (recenterTargetPitch - recenterStartPitch) * s

            applyLook()
            return
        end

        local gx, gy, gz = Input.GetGyroscopeRotatioRate()

        yaw   = yaw   + math.deg(gx) * dt * gyroSensitivity
        pitch = pitch - math.deg(gy) * dt * gyroSensitivity

        applyLook()
    end
end

-- ============================================================
function on_destroy()
    if PLATFORM == "windows" then
        Input.SetCursorLocked(false)
        Input.SetCursorVisible(true)
    end
    print("[player_pov_camera] Destroyed (entity " .. tostring(EntityID) .. ")")
end
