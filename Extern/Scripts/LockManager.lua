----------------------------------------------------------------
-- LockManager.lua
-- Handles the combination lock mechanic on the toolbox.
-- Correct combo: digits {0, 1, 2, 4} (order does not matter)
--
-- Attach to the toolbox entity (same entity as ToolboxOpenMechanic).
-- Button events from LockScreen number buttons drive this script.
----------------------------------------------------------------

local COMBO      = { [0]=true, [9]=true, [1]=true, [3]=true }
local MAX_FAILS  = 3
local UI_TABLE   = "LOCKSCREEN_OPEN"

local TOOLBOX_LOCKED_ID = 479   -- closed toolbox (hide on success)
local TOOLBOX_OPEN_ID   = 480   -- open toolbox   (show on success)
local BOLT_CUTTER_ID    = 481   -- bolt cutter    (move into toolbox position on success)

-- Light entities dimmed on each fail (fail 1 -> 485, fail 2 -> 484, fail 3 -> 483)
local FAIL_LIGHTS = { 485, 484, 483 }

-- Position of the closed toolbox — bolt cutter moves here on success
local TOOLBOX_POS_X = 642.8716430664063
local TOOLBOX_POS_Y = 68.80591583251953
local TOOLBOX_POS_Z = 235.96548461914063

local pressed            = {}   -- set of currently pressed digit indices
local pressed_count      = 0
local was_open           = false
local do_close           = false   -- deferred: close on next on_update
local do_success         = false   -- deferred: success on next on_update
local pending_fail_light = nil     -- light entity to blow out during cinematic

-- -----------------------------------------------
-- Candle cinematic state
-- -----------------------------------------------
local ZOOM_IN_TIME  = 1.0   -- seconds to pan toward candle
local HOLD_TIME     = 0.5   -- seconds to hold on blown-out candle
local ZOOM_OUT_TIME = 0.8   -- seconds to pan back
local ZOOM_DIST     = 60.0  -- how far from candle the zoom-in camera sits
local ZOOM_FOV      = 30.0  -- FOV at peak zoom (smaller = more zoomed in)

local cinematic_phase = 0   -- 0=off  1=zoom_in  2=hold  3=zoom_out
local cinematic_timer = 0.0
local cinematic_cam   = nil

-- Saved camera state at cinematic start
local c_from_x,  c_from_y,  c_from_z  = 0, 0, 0
local c_from_tx, c_from_ty, c_from_tz = 0, 0, 0
local c_from_fov = 60.0

-- Candle world position and computed zoom-in camera position
local c_candle_x, c_candle_y, c_candle_z = 0, 0, 0
local c_zoom_x,   c_zoom_y,   c_zoom_z   = 0, 0, 0

local function lerp(a, b, t) return a + (b - a) * t end
local function ease_in_out(t) return t * t * (3.0 - 2.0 * t) end

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

local function play_sfx(entityId, slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(entityId, slotId)
    end
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

            -- Queue the candle for the cinematic (do NOT dim yet)
            local lightId = FAIL_LIGHTS[SHARED.LOCK_FAIL_COUNT]
            if lightId then
                pending_fail_light = lightId
            end

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

    -- Pre-position bolt cutter at the toolbox now (hidden at scale 0) so its
    -- physics trigger is registered before the player arrives. Teleporting it
    -- on success skips Jolt's trigger_enter detection entirely.
    local boltT = Query.GetTransform(BOLT_CUTTER_ID)
    if boltT then
        boltT:SetPosition(TOOLBOX_POS_X, TOOLBOX_POS_Y, TOOLBOX_POS_Z)
        boltT:SetScale(0, 0, 0)
    end
end

function on_update(self, dt)
    local open = is_open()

    local FAIL_GLOW_LIGHTS = {
        [485] = 689,
        [484] = 846,
        [483] = 883,
    }

    SHARED = SHARED or {}
    SHARED.STOREROOM_FLICKER_OFF = SHARED.STOREROOM_FLICKER_OFF or {}

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
        if PLATFORM == "android" then
            UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
            UI_DisplayMenu("AndroidInputOverlay")
        end

        -- Swap toolbox meshes
        local lockedT = Query.GetTransform(TOOLBOX_LOCKED_ID)
        local openT   = Query.GetTransform(TOOLBOX_OPEN_ID)
        local boltT   = Query.GetTransform(BOLT_CUTTER_ID)

        if lockedT then lockedT:SetPosition(0, -99999, 0) end
        if openT   then openT:SetScale(1, 1, 1) end
        if boltT   then boltT:SetScale(1, 1, 1) end

        -- Remove locked toolbox from interaction tracking so it no longer
        -- steals E-press commands from the bolt cutter
        if SHARED.INTERACTABLE then SHARED.INTERACTABLE[TOOLBOX_LOCKED_ID] = nil end
        if SHARED.INTERACT_IN_ZONE then SHARED.INTERACT_IN_ZONE[TOOLBOX_LOCKED_ID] = nil end

        print("[LockManager] Lock opened successfully!")
        return
    end

    if do_close then
        do_close         = false
        SHARED[UI_TABLE] = false
        reset_pressed()
        UI_CloseAllMenus()
        if PLATFORM == "android" then
            UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
            UI_DisplayMenu("AndroidInputOverlay")
        end

        -- Start candle cinematic if there is a candle to blow out
        if pending_fail_light then
            local cam     = GetMainCamera and GetMainCamera()
            local candleT = Query.GetTransform(pending_fail_light)

            if cam and candleT then
                -- Save current camera state
                c_from_x, c_from_y, c_from_z   = cam:GetPosition()
                c_from_tx, c_from_ty, c_from_tz = cam:GetTarget()
                c_from_fov                       = cam:GetFOV()

                -- Candle world position
                c_candle_x, c_candle_y, c_candle_z = candleT:GetPosition()

                -- Place zoom-in camera ZOOM_DIST units away from candle,
                -- in the same direction the player camera is already looking from
                local dx = c_from_x - c_candle_x
                local dy = c_from_y - c_candle_y
                local dz = c_from_z - c_candle_z
                local d  = math.sqrt(dx*dx + dy*dy + dz*dz)
                if d < 0.001 then d = 1.0 end
                c_zoom_x = c_candle_x + (dx / d) * ZOOM_DIST
                c_zoom_y = c_candle_y + (dy / d) * ZOOM_DIST
                c_zoom_z = c_candle_z + (dz / d) * ZOOM_DIST

                cinematic_cam   = cam
                cinematic_phase = 1
                cinematic_timer = 0.0
                SHARED.CINEMATIC_CAMERA = true
            else
                -- No camera or candle transform — blow out immediately
                local light = Query.GetLight(pending_fail_light)
                if light then light:SetIntensity(0) end

                local glowId = FAIL_GLOW_LIGHTS[pending_fail_light]
                if glowId then
                    SHARED.STOREROOM_FLICKER_OFF[glowId] = true
                    print("[LockManager] Queued glow light OFF for entity " .. tostring(glowId))
                end

                play_sfx(pending_fail_light, "Candles")
                pending_fail_light = nil
            end
        end

        print("[LockManager] Wrong combo. Fails: " .. tostring(SHARED.LOCK_FAIL_COUNT))
    end

    -- -----------------------------------------------
    -- Candle cinematic update
    -- -----------------------------------------------
    if cinematic_phase > 0 and cinematic_cam then
        cinematic_timer = cinematic_timer + dt

        if cinematic_phase == 1 then
            -- Pan toward candle and zoom FOV in
            local t  = ease_in_out(math.min(cinematic_timer / ZOOM_IN_TIME, 1.0))
            cinematic_cam:SetPosition(
                lerp(c_from_x, c_zoom_x, t),
                lerp(c_from_y, c_zoom_y, t),
                lerp(c_from_z, c_zoom_z, t)
            )
            cinematic_cam:LookAt(c_candle_x, c_candle_y, c_candle_z)
            cinematic_cam:SetFOV(lerp(c_from_fov, ZOOM_FOV, t))

            if cinematic_timer >= ZOOM_IN_TIME then
                cinematic_phase = 2
                cinematic_timer = 0.0
                -- Blow out the candle at peak zoom
                local light = Query.GetLight(pending_fail_light)
                if light then light:SetIntensity(0) end

                local glowId = FAIL_GLOW_LIGHTS[pending_fail_light]
                if glowId then
                    SHARED.STOREROOM_FLICKER_OFF[glowId] = true
                    print("[LockManager] Queued glow light OFF for entity " .. tostring(glowId))
                end

                play_sfx(pending_fail_light, "Candles")
            end

        elseif cinematic_phase == 2 then
            -- Hold on the dark candle
            cinematic_cam:SetPosition(c_zoom_x, c_zoom_y, c_zoom_z)
            cinematic_cam:LookAt(c_candle_x, c_candle_y, c_candle_z)
            cinematic_cam:SetFOV(ZOOM_FOV)

            if cinematic_timer >= HOLD_TIME then
                cinematic_phase = 3
                cinematic_timer = 0.0
            end

        elseif cinematic_phase == 3 then
            -- Pan back to original camera position
            local t = ease_in_out(math.min(cinematic_timer / ZOOM_OUT_TIME, 1.0))
            cinematic_cam:SetPosition(
                lerp(c_zoom_x, c_from_x, t),
                lerp(c_zoom_y, c_from_y, t),
                lerp(c_zoom_z, c_from_z, t)
            )
            cinematic_cam:LookAt(
                lerp(c_candle_x, c_from_tx, t),
                lerp(c_candle_y, c_from_ty, t),
                lerp(c_candle_z, c_from_tz, t)
            )
            cinematic_cam:SetFOV(lerp(ZOOM_FOV, c_from_fov, t))

            if cinematic_timer >= ZOOM_OUT_TIME then
                -- Restore exactly and hand control back to camera script
                cinematic_cam   = nil
                cinematic_phase = 0
                pending_fail_light = nil
                SHARED.CINEMATIC_CAMERA = false

                -- After 3rd candle: trigger the girl jumpscare
                if SHARED.LOCK_FAIL_COUNT >= MAX_FAILS then
                    SHARED.TOOLBOX_JUMPSCARE = true
                    print("[LockManager] All candles out -> TOOLBOX_JUMPSCARE")
                end
            end
        end
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
