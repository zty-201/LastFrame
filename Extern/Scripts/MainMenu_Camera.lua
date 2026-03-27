-- MainMenuCamera.lua
-- Attach to a dedicated MainMenu_Camera entity placed in the bedroom.
-- While GAME_STATE == "MENU", this script drives the main camera.

local DEBUG = true

-- Optional: assign a look target entity in the scene.
-- Leave as 0 to use fixed yaw/pitch instead.
local LOOK_TARGET_ENTITY_ID = 0

-- Only used if LOOK_TARGET_ENTITY_ID == 0
local FIXED_YAW_DEG   = 125.0
local FIXED_PITCH_DEG = 0.0

local camera = nil
local transform = nil

local function apply_fixed_look(x, y, z)
    local radYaw   = math.rad(FIXED_YAW_DEG)
    local radPitch = math.rad(FIXED_PITCH_DEG)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    camera:SetPosition(x, y, z)
    camera:LookAt(x + fx, y + fy, z + fz)
end

local function apply_target_look(x, y, z, targetId)
    local t = Query.GetTransform(targetId)
    if t == nil then
        apply_fixed_look(x, y, z)
        return
    end

    local tx, ty, tz = t:GetPosition()
    camera:SetPosition(x, y, z)
    camera:LookAt(tx, ty, tz)
end

function on_create(self)
    transform = EntityTransform
    camera = GetMainCamera()

    if not transform then
        print("[MainMenuCamera] ERROR: Transform missing")
        return
    end

    if not camera then
        print("[MainMenuCamera] ERROR: Camera missing")
        return
    end

    if DEBUG then
        print("[MainMenuCamera] Ready on EntityID=" .. tostring(EntityID))
    end
end

function on_update(self, dt)
    if not transform or not camera then return end

    if _G.GAME_STATE ~= "MENU" then
        return
    end

    local x, y, z = transform:GetPosition()

    if LOOK_TARGET_ENTITY_ID ~= 0 then
        apply_target_look(x, y, z, LOOK_TARGET_ENTITY_ID)
    else
        apply_fixed_look(x, y, z)
    end
end

function on_destroy(self)
    if DEBUG then
        print("[MainMenuCamera] Destroyed EntityID=" .. tostring(EntityID))
    end
end