----------------------------------------------------------------
-- Credits.lua
-- Attach to any persistent entity (e.g. 1014).
-- Controls entity 13 (CreditsPng) inside CreditsMenu.
--
-- Detects when the StateMachine enters "EndCredit" and:
--   1. Resets position_NDC.y to START_Y  (-9)  every time (no leftover state)
--   2. Scrolls upward at SPEED NDC/s
--   3. When y >= END_Y (9) fires ReturnToMenu
----------------------------------------------------------------

local CREDITS_STATE  = "EndCredit"
local CREDITS_ENTITY = 13          -- entity with the Credits.png UIComponent
local START_Y        = -9.0
local END_Y          =  9.0
local SPEED          = 0.3         -- NDC units per second (tweak to taste)

local wasInCredits   = false
local playing        = false

----------------------------------------------------------------

function on_create(self)
    wasInCredits = false
    playing      = false
    print("[Credits] Script loaded on entity " .. tostring(EntityID))
end

function on_update(self, dt)
    dt = dt or 0.016

    local state     = StateMachine_GetCurrentStateName and StateMachine_GetCurrentStateName() or ""
    local inCredits = (state == CREDITS_STATE)

    -- Detect fresh entry into credits state
    if inCredits and not wasInCredits then
        wasInCredits = true
        playing      = true

        -- Always reset to the top so replaying starts fresh
        local img = GetEntityUI(CREDITS_ENTITY)
        if img then
            local x, _ = img:GetPosition_NDC()
            img:SetPosition_NDC(x, START_Y)
        end

        print("[Credits] Entering EndCredit -> reset y to " .. START_Y)
        return
    end

    -- Detect exit from credits state (e.g. stopped mid-play in editor)
    if not inCredits then
        if wasInCredits then
            print("[Credits] Left EndCredit state -> stopping")
        end
        wasInCredits = false
        playing      = false
        return
    end

    if not playing then return end

    -- Scroll upward
    local img = GetEntityUI(CREDITS_ENTITY)
    if not img then return end

    local x, y = img:GetPosition_NDC()
    y = y + SPEED * dt
    img:SetPosition_NDC(x, y)

    -- Finished
    if y >= END_Y then
        playing = false
        print("[Credits] Reached END_Y (" .. END_Y .. ") -> ReturnToMenu")
        if StateMachine_SetTrigger then
            StateMachine_SetTrigger("ReturnToMenu")
        end
    end
end

function on_destroy(self)
    playing      = false
    wasInCredits = false
end
