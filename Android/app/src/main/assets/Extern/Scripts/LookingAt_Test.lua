-- LookingAt_Debug.lua
-- Uses CAMERA forward (target - position) + checks if camera is looking at TARGET_ID within FOV.
-- Requires your new bindings:
--   Query.GetCameraForward(entityId)
--   Query.IsCameraLookingAt(cameraEntityId, targetEntityId, fovDegrees)
--
-- Notes:
-- - This script assumes EntityID is the CAMERA entity (the one that has CameraComponent).
-- - TARGET_ID should be an entity that has TransformComponent.

local TARGET_ID = 23
local FOV = 30.0
local timer = 0

local function clamp(x, a, b)
    if x < a then return a end
    if x > b then return b end
    return x
end

local function dot(ax, ay, az, bx, by, bz)
    return ax * bx + ay * by + az * bz
end

local function len(ax, ay, az)
    return math.sqrt(ax * ax + ay * ay + az * az)
end

local function normalize(x, y, z)
    local l = len(x, y, z)
    if l < 0.000001 then return 0, 0, 0, 0 end
    return x / l, y / l, z / l, l
end

function on_create(Self)
    print("[LookingAt_Debug] CameraEntityID=", EntityID, "Target=", TARGET_ID, "FOV=", FOV)
end

function on_update(Self, dt)
    timer = timer + dt
    if timer < 0.5 then return end
    timer = 0

    -- ✅ Use camera-based version
    local ok = Query.IsCameraLookingAt(EntityID, TARGET_ID, FOV)

    -- Camera entity should have CameraComponent. We use GetMainCamera() (if you have it),
    -- otherwise you can add Query.GetCamera(EntityID) in C++ later.
    local cam = GetMainCamera()
    if cam == nil then
        print("[LookingAt_Debug] No main camera found (GetMainCamera() returned nil)")
        return
    end

    -- Target needs Transform
    local b = Query.GetTransform(TARGET_ID)
    if b == nil then
        print("[LookingAt_Debug] Target has no Transform (TARGET_ID=", TARGET_ID, ")")
        return
    end

    local cx, cy, cz = cam:GetPosition()
    local bx, by, bz = b:GetPosition()

    -- ✅ Camera forward from C++
    local fx, fy, fz = Query.GetCameraForward(EntityID)

    -- Vector from camera -> target
    local tx, ty, tz = (bx - cx), (by - cy), (bz - cz)
    tx, ty, tz, tlen = normalize(tx, ty, tz)

    if tlen < 0.0001 then
        print("[LookingAt_Debug] Target is basically same position as camera -> ok=", ok)
        return
    end

    -- dot(fwd, toTarget)
    local c = dot(fx, fy, fz, tx, ty, tz)
    c = clamp(c, -1, 1)

    -- threshold used in C++ = cos(radians(FOV*0.5))
    local thresh = math.cos(math.rad(FOV * 0.5))

    print(string.format(
        "[LookingAt_Debug] ok=%s dot=%.3f thresh=%.3f  camPos(%.2f %.2f %.2f) fwd(%.2f %.2f %.2f) toT(%.2f %.2f %.2f)",
        tostring(ok), c, thresh, cx, cy, cz, fx, fy, fz, tx, ty, tz
    ))
end
