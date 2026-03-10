-- Playroom_FanRotate.lua
-- Specific logic for the playroom fan.
-- Rotates only during CP1.
-- When CP1 ends, the fan slows down smoothly to a stop.

print("[Playroom_FanRotate] SCRIPT LOADED")

local DEBUG = false

local STATE_IDLE = 0
local STATE_CP1  = 1
local STATE_CP2  = 2
local STATE_CP3  = 3

-- Max spin speed
local MAX_SPIN_SPEED = 180.0

-- How quickly the fan slows down
local SPIN_DECELERATION = 90.0   -- degrees/sec²

function on_create(self)

    print("[Playroom_FanRotate] on_create EntityID =", EntityID)

    self._angle = 0.0
    self._speed = 0.0

    self._baseX = 0.0
    self._baseY = 0.0
    self._baseZ = 0.0

    local tr = Query.GetTransform(EntityID)
    if tr ~= nil then
        local rx, ry, rz = tr:GetRotation()
        self._baseX = rx
        self._baseY = ry
        self._baseZ = rz
        self._angle = ry
    end
end

local function should_spin()
    local controller = SHARED and SHARED.StateController and SHARED.StateController._self
    if not controller then
        return false
    end

    return controller.state == STATE_CP1
end

function on_update(self, dt)

    if dt == nil then dt = 0.016 end

    ------------------------------------------------
    -- Update speed
    ------------------------------------------------
    if should_spin() then
        -- accelerate instantly to max speed
        self._speed = MAX_SPIN_SPEED
    else
        -- slow down smoothly
        if self._speed > 0 then
            self._speed = self._speed - SPIN_DECELERATION * dt
            if self._speed < 0 then
                self._speed = 0
            end
        end
    end

    ------------------------------------------------
    -- Apply rotation if moving
    ------------------------------------------------
    if self._speed > 0 then

        self._angle = self._angle + self._speed * dt

        if self._angle >= 360.0 then
            self._angle = self._angle - 360.0
        end

        if Transform_SetRotation ~= nil then
            Transform_SetRotation(EntityID, self._baseX, self._angle, self._baseZ)
            return
        end

        local tr = Query.GetTransform(EntityID)
        if tr ~= nil then
            tr:SetRotation(self._baseX, self._angle, self._baseZ)
        end
    end
end

function on_destroy(self)
    print("[Playroom_FanRotate] on_destroy EntityID =", EntityID)
end