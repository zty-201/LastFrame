----------------------------------------------------------------
-- Test_AddPlayroomKey_OnE.lua
-- Press E (GLFW) + player in range -> add key + shrink entity 205
----------------------------------------------------------------

local DEBUG = true
local KEY_INTERACT = GLFW_KEY_E   -- GLFW E key

local ITEM_ID   = "playroom_key"
local ITEM_NAME = "Playroom Key"
local ITEM_QTY  = 1

local TARGET_ENTITY_ID = 205      -- entity to shrink / "pickup"
local PLAYER_ID = 5              -- your confirmed player id
local INTERACT_RANGE = 200.0

function on_create(Self)
    if DEBUG then
        print("[Test_AddKey] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Add then
            print("[Test_AddKey] WARNING: Inventory not ready (SHARED.Inventory.Add missing)")
        end
    end
end

function on_update(Self, dt)
    if Input.GetKeyDown(KEY_INTERACT) then
        local inRange = Query.IsEntityInRange(TARGET_ENTITY_ID, PLAYER_ID, INTERACT_RANGE)

        if DEBUG then
            print(string.format(
                "[Test_AddKey] E pressed. inRange=%s (range=%.1f)",
                tostring(inRange),
                INTERACT_RANGE
            ))
        end

        if not inRange then
            return
        end

        -- 1️⃣ Add key to inventory
        if SHARED.Inventory and SHARED.Inventory.Add then
            SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)

            if DEBUG then
                print("[Test_AddKey] Added:", ITEM_NAME, "x" .. ITEM_QTY)
            end
        else
            print("[Test_AddKey] ERROR: Inventory not available")
        end

        -- 2️⃣ Shrink entity 205
        local tr = Query.GetTransform(TARGET_ENTITY_ID)
        if tr then
            tr:SetScale(0.0, 0.0, 0.0)
            if DEBUG then
                print("[Test_AddKey] Shrunk entity:", TARGET_ENTITY_ID)
            end
        else
            print("[Test_AddKey] ERROR: No Transform for entity:", TARGET_ENTITY_ID)
        end
    end
end
