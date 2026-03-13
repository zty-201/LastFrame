----------------------------------------------------------------
-- GameOver_Screen.lua
-- Attach to entity 1002 (Game_Over_Screen).
-- Entity 1004 is the black screen used for the fade.
--
-- Sequence after SHARED.PLAYER_DEAD is set:
--   1. Wait 1s
--   2. Fade in black screen (entity 1004) over FADE_DURATION
--   3. Show game over screen (entity 1002)
--   4. Hold 3s
--   5. Return to main menu
----------------------------------------------------------------

local BLACK_SCREEN_ID = 1004
local WAIT_DURATION   = 0.6
local FADE_DURATION   = 0.8
local HOLD_DURATION   = 3.0

local STATE_IDLE      = 0
local STATE_WAIT      = 1
local STATE_FADE      = 2
local STATE_HOLD      = 3

local state = STATE_IDLE
local timer = 0.0

local function black_screen()
    return GetEntityUI(BLACK_SCREEN_ID)
end

function on_create(self)
    state = STATE_IDLE
    timer = 0.0
    -- Clear flag so a scene restart doesn't immediately re-trigger
    local S = rawget(_G, "SHARED")
    if S then S.PLAYER_DEAD = false end

    if EntityUI then EntityUI:SetVisible(false) end
    local bs = black_screen()
    if bs then
        bs:SetVisible(false)
        bs:SetColor(0, 0, 0, 0)
    end
end

function on_update(self, dt)
    if state == STATE_IDLE then
        local S = rawget(_G, "SHARED")
        if S and S.PLAYER_DEAD then
            state = STATE_WAIT
            timer = 0.0
        end
        return
    end

    timer = timer + dt

    if state == STATE_WAIT then
        if timer >= WAIT_DURATION then
            state = STATE_FADE
            timer = 0.0
            local bs = black_screen()
            if bs then
                bs:SetVisible(true)
                bs:SetColor(0, 0, 0, 0)
            end
        end

    elseif state == STATE_FADE then
        local t = math.min(timer / FADE_DURATION, 1.0)
        local bs = black_screen()
        if bs then bs:SetColor(0, 0, 0, t) end

        if timer >= FADE_DURATION then
            -- Black is fully opaque — show game over screen underneath
            if EntityUI then EntityUI:SetVisible(true) end
            state = STATE_HOLD
            timer = 0.0
        end

    elseif state == STATE_HOLD then
        if timer >= HOLD_DURATION then
            if EntityUI then EntityUI:SetVisible(false) end
            local bs = black_screen()
            if bs then bs:SetVisible(false) end

            SHARED = SHARED or {}
            SHARED.PLAYER_DEAD                = false
            SHARED.UI_INVENTORY_OPEN          = false
            SHARED.UI_INVENTORY_ANDROID_STATE = false
            _G.GAME_STATE = "MENU"
            if StateMachine_SetTrigger then StateMachine_SetTrigger("ReturnToMenu") end
            Scene_RequestRestart()
            if UI_CloseAllMenus then UI_CloseAllMenus() end
        end
    end
end

function on_destroy(self)
    state = STATE_IDLE
    timer = 0.0
end
