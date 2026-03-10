----------------------------------------------------------------
-- LockManager.lua
-- Handles the combination lock mechanic on the toolbox.
-- Correct combo: digits {0, 1, 2, 4} (order does not matter)
--
-- Attach to the toolbox entity (same entity as ToolboxOpenMechanic).
-- Button events from LockScreen number buttons drive this script.
----------------------------------------------------------------

local COMBO      = { [0]=true, [1]=true, [2]=true, [4]=true }
local MAX_FAILS  = 3
local UI_TABLE   = "LOCKSCREEN_OPEN"

local TOOLBOX_LOCKED_ID = 479   -- closed toolbox (hide on success)
local TOOLBOX_OPEN_ID   = 480   -- open toolbox   (show on success)
local BOLT_CUTTER_ID    = 481   -- bolt cutter    (move into toolbox position on success)

-- Position of the closed toolbox — bolt cutter moves here on success
local TOOLBOX_POS_X = 642.8716430664063
local TOOLBOX_POS_Y = 68.80591583251953
local TOOLBOX_POS_Z = 235.96548461914063

local pressed       = {}   -- set of currently pressed digit indices
local pressed_count = 0
local was_open      = false
local do_close      = false   -- deferred: close on next on_update
local do_success    = false   -- deferred: success on next on_update

-- -----------------------------------------------
-- Helpers
-- -----------------------------------------------
local function reset_pressed()
    pressed       = {}
    pressed_count = 0
end

local function is_open()
    return SHARED and SHARED[UI_TABLE] == true
end

local function handle_digit(digit)
    if not is_open() then return end
    if SHARED.LOCK_DISABLED then return end

    -- Toggle the digit in our tracking set
    if pressed[digit] then
        pressed[digit] = nil
        pressed_count  = pressed_count - 1
    else
        pressed[digit] = true
        pressed_count  = pressed_count + 1
    end

    -- Auto-check triggers exactly when 4 buttons are pressed
    if pressed_count == 4 then
        local correct = true
        for req, _ in pairs(COMBO) do
            if not pressed[req] then
                correct = false
                break
            end
        end

        if correct then
            do_success = true
        else
            -- Wrong combo -> failure
            SHARED.LOCK_FAIL_COUNT = (SHARED.LOCK_FAIL_COUNT or 0) + 1
            if SHARED.LOCK_FAIL_COUNT >= MAX_FAILS then
                SHARED.LOCK_DISABLED = true
            end
            reset_pressed()
            do_close = true
        end
    end
end

-- -----------------------------------------------
-- Lifecycle
-- -----------------------------------------------
function on_create(self)
    SHARED[UI_TABLE]       = false
    SHARED.LOCK_FAIL_COUNT = 0
    SHARED.LOCK_DISABLED   = false
    reset_pressed()
end

function on_update(self, dt)
    local open = is_open()

    -- Reset tracking when overlay is freshly opened
    if open and not was_open then
        reset_pressed()
    end
    was_open = open

    if do_success then
        do_success           = false
        SHARED[UI_TABLE]     = false
        SHARED.LOCK_DISABLED = true
        reset_pressed()
        UI_CloseAllMenus()

        -- Swap toolbox meshes
        local lockedT = Query.GetTransform(TOOLBOX_LOCKED_ID)
        local openT   = Query.GetTransform(TOOLBOX_OPEN_ID)
        local boltT   = Query.GetTransform(BOLT_CUTTER_ID)

        if lockedT then lockedT:SetPosition(0, -99999, 0) end          -- hide closed toolbox
        if openT   then openT:SetScale(1, 1, 1) end                    -- reveal open toolbox
        if boltT   then boltT:SetPosition(TOOLBOX_POS_X, TOOLBOX_POS_Y, TOOLBOX_POS_Z) end  -- place bolt cutter

        print("[LockManager] Lock opened successfully!")
        return
    end

    if do_close then
        do_close         = false
        SHARED[UI_TABLE] = false
        reset_pressed()
        UI_CloseAllMenus()
        print("[LockManager] Wrong combo. Fails: " .. tostring(SHARED.LOCK_FAIL_COUNT))
    end
end

-- -----------------------------------------------
-- Button event handlers (fired by ButtonComponent event_trigger)
-- -----------------------------------------------
function on_button_LockNum_0(self) handle_digit(0) end
function on_button_LockNum_1(self) handle_digit(1) end
function on_button_LockNum_2(self) handle_digit(2) end
function on_button_LockNum_3(self) handle_digit(3) end
function on_button_LockNum_4(self) handle_digit(4) end
function on_button_LockNum_5(self) handle_digit(5) end
function on_button_LockNum_6(self) handle_digit(6) end
function on_button_LockNum_7(self) handle_digit(7) end
function on_button_LockNum_8(self) handle_digit(8) end
function on_button_LockNum_9(self) handle_digit(9) end
