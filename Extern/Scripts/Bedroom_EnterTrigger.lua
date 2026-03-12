-- Bedroom_EnterTrigger.lua
-- Attach to the bedroom trigger/hitbox entity (must also have Interactable_Register2.0).
-- When the player enters proximity, plays laughter audio on the girl entity.
-- Fires once only.

local GIRL_ENTITY_ID = 251
local LAUGH_SLOT     = "GirlLaughing"

local fired      = false
local wasInRange = false

function on_create(self)
    fired      = false
    wasInRange = false
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    print("[Bedroom_EnterTrigger] Ready on entity " .. tostring(EntityID))
end

function on_update(self, dt)
    if fired then return end

    local inRange = SHARED.INTERACT_IN_ZONE
                    and SHARED.INTERACT_IN_ZONE[EntityID] == true

    if inRange and not wasInRange then
        fired = true
        Audio_PlaySlot(GIRL_ENTITY_ID, LAUGH_SLOT)
        print("[Bedroom_EnterTrigger] TRIGGERED -> playing " .. LAUGH_SLOT .. " on entity " .. GIRL_ENTITY_ID)
    end

    wasInRange = inRange
end

function on_destroy(self)
    fired      = false
    wasInRange = false
end
