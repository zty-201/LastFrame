----------------------------------------------------------------
-- Credits.lua
-- Attach to any persistent entity.
--
-- Controls 6 separate credit UI entities:
--   Credits_1
--   Credits_2
--   Credits_3
--   Credits_4
--   Credits_5
--   Credits_6
--
-- Detects when the StateMachine enters "EndCredit" and:
--   1. Resets all 6 images into one continuous vertical sequence
--   2. Scrolls them upward together
--   3. When the last image has fully passed END_Y, fires ReturnToMenu
----------------------------------------------------------------

local CREDITS_STATE = "EndCredit"

local CREDIT_ENTITIES = {
    700, -- Credits_1
    395, -- Credits_2
    397, -- Credits_3
    698, -- Credits_4
    701, -- Credits_5
    699  -- Credits_6
}

-- Where the FIRST credit starts
local START_Y = 0.0

-- When the LAST credit passes this, credits end
local END_Y   =  2.0

local SPEED   =  0.3

-- IMPORTANT:
-- This controls how closely each credit "attaches" to the next one.
-- If your images overlap, increase this.
-- If the gap is too large, decrease this.
--
-- Since Y increases upward, the next image is placed BELOW the previous one,
-- so we subtract this value each time.
local CREDIT_SPACING = 2.0

local wasInCredits = false
local playing      = false
local creditData   = {}

----------------------------------------------------------------

function on_create(self)
    wasInCredits = false
    playing      = false
    creditData   = {}
    print("[Credits] Script loaded on entity " .. tostring(EntityID))
end

local function ResetCreditsPositions()
    creditData = {}

    local currentY = START_Y

    for i = 1, #CREDIT_ENTITIES do
        local entityID = CREDIT_ENTITIES[i]
        local img = GetEntityUI(entityID)

        if img then
            -- Force center X = 0
            local x = 0.0

            img:SetPosition_NDC(x, currentY)

            creditData[i] = {
                entityID = entityID,
                x = x,
                y = currentY
            }

            -- Stack next credit BELOW
            currentY = currentY - CREDIT_SPACING
        else
            print("[Credits] Warning: missing UI entity ID " .. tostring(entityID))
        end
    end
end

local function UpdateCredits(dt)
    local lastCredit = creditData[#creditData]
    if not lastCredit then
        return true
    end

    for i = 1, #creditData do
        local data = creditData[i]
        local img = GetEntityUI(data.entityID)

        if img then
            data.y = data.y + SPEED * dt
            img:SetPosition_NDC(data.x, data.y)
        end
    end

    -- End only after the LAST credit has moved out of screen
    return (lastCredit.y >= END_Y)
end

local function FinishCredits()
    playing = false
    print("[Credits] Final credit finished -> ReturnToMenu")

    SHARED = SHARED or {}
    SHARED.PLAYER_DEAD                = false
    SHARED.UI_INVENTORY_OPEN          = false
    SHARED.UI_INVENTORY_ANDROID_STATE = false
    SHARED.EVIDENCE_FOLDER_FINISHED   = false
    _G.GAME_STATE = "MENU"

    if Scene_RequestRestart then Scene_RequestRestart() end
    if UI_CloseAllMenus    then UI_CloseAllMenus()     end
    if StateMachine_SetTrigger then
        StateMachine_SetTrigger("ReturnToMenu")
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    local state     = StateMachine_GetCurrentStateName and StateMachine_GetCurrentStateName() or ""
    local inCredits = (state == CREDITS_STATE)

    -- Detect fresh entry into credits state
    if inCredits and not wasInCredits then
        wasInCredits = true
        playing      = true

        ResetCreditsPositions()

        print("[Credits] Entering EndCredit -> reset continuous credit roll")
        return
    end

    -- Detect exit from credits state
    if not inCredits then
        if wasInCredits then
            print("[Credits] Left EndCredit state -> stopping")
        end
        wasInCredits = false
        playing      = false
        return
    end

    if not playing then return end

    local finished = UpdateCredits(dt)

    if finished then
        FinishCredits()
    end
end

function on_destroy(self)
    playing      = false
    wasInCredits = false
    creditData   = {}
end