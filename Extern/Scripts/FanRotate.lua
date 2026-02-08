-- FanRotate.lua
-- Attach to a fan entity. Spins it continuously over time.

print("[FanRotate] SCRIPT LOADED")

-- Degrees per second (tweak this in code if you don't have inspector vars)
local SPIN_SPEED_DPS = 60.0

function on_create(self)
    print("[FanRotate] on_create EntityID =", EntityID)
    self._angle = 0.0
end

function on_update(self, dt)
    if dt == nil then dt = 0.016 end

    -- accumulate rotation
    self._angle = self._angle + SPIN_SPEED_DPS * dt

    -- keep value bounded
    if self._angle >= 360.0 then
        self._angle = self._angle - 360.0
    end

    -- Apply rotation (pick the binding your engine exposes)
    if Transform_SetRotation ~= nil then
        Transform_SetRotation(EntityID, 0.0, self._angle, 0.0)
        return
    end
end

function on_destroy(self)
    print("[FanRotate] on_destroy EntityID =", EntityID)
end
