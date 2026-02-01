----------------------------------------------------------------
-- Test_ConsumeMusicDisc_OnE.lua
-- Press Q (GLFW) -> consume 1 Music Disc from inventory
-- If consumed successfully -> trigger StateController CP1
----------------------------------------------------------------

local DEBUG = true
local KEY_INTERACT = GLFW_KEY_Q

local ITEM_ID = "music_disc"
local ITEM_QTY = 1

function on_create(Self)
    if DEBUG then
        print("[Test_ConsumeDisc] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Remove then
            print("[Test_ConsumeDisc] WARNING: Inventory.Remove not found")
        end
    end
end

function on_update(Self, dt)
    if Input.GetKeyDown(KEY_INTERACT) then
        if not SHARED.Inventory or not SHARED.Inventory.Remove then
            print("[Test_ConsumeDisc] ERROR: Inventory not available")
            return
        end

        local count = 0
        if SHARED.Inventory.Count then
            count = SHARED.Inventory.Count(ITEM_ID)
        end

        if count > 0 then
            SHARED.Inventory.Remove(ITEM_ID, ITEM_QTY)

            if DEBUG then
                print("[Test_ConsumeDisc] Consumed:", ITEM_ID, "x" .. ITEM_QTY)
                if SHARED.Inventory.Print then
                    SHARED.Inventory.Print()
                end
            end

            if SHARED.StateController and SHARED.StateController.TriggerCP1 then
                SHARED.StateController.TriggerCP1()
            else
                print("[Test_ConsumeDisc] WARNING: StateController trigger not found")
            end
        else
            if DEBUG then
                print("[Test_ConsumeDisc] No Music Disc to consume")
            end
        end
    end
end
