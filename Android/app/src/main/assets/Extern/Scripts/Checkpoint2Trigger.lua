----------------------------------------------------------------
-- Trigger_CP2_WhenInInteractionZone.lua
-- When INTERACT_IN_ZONE[643] becomes true (player's InteractionBox
-- is in range of interactable 643), trigger Checkpoint 2 ONCE.
----------------------------------------------------------------

local DEBUG = true

-- Interactable / item ID that InteractionBox reports
local INTERACTABLE_ID = 643

local triggeredOnce = false

local function in_interaction_zone()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[INTERACTABLE_ID] == true
end

function on_create(self)
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}

    if DEBUG then
        print("[Trigger_CP2] watching INTERACTABLE_ID =", INTERACTABLE_ID)
    end
end

function on_update(self, dt)
    if triggeredOnce then return end

    if in_interaction_zone() then
        triggeredOnce = true

        if SHARED.StateController and SHARED.StateController.TriggerCP2 then
            SHARED.StateController.TriggerCP2()

            if DEBUG then
                print("[Trigger_CP2] INTERACT_IN_ZONE true -> TriggerCP2()")
            end
        else
            print("[Trigger_CP2] ERROR: StateController.TriggerCP2 not found")
        end
    end
end
