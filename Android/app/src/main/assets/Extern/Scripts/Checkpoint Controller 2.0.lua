----------------------------------------------------------------
-- StateController_CP1_CP2.lua
-- CP1: Play audio once, wait, then shrink hidden door + scatter chair parts
-- CP2: Play audio once, wait, then run CP2 actions (stubbed)
----------------------------------------------------------------

local DEBUG = true

local STATE_IDLE = 0
local STATE_CP1  = 1
local STATE_CP2  = 2

-- =========================
-- CP1 CONFIG
-- =========================
local SONG1_SLOT_ID = "Checkpoint1"
local SONG1_LENGTH_SECONDS = 10.0
local HIDDEN_DOOR_ID = 43

-- =========================
-- CP2 CONFIG (EDIT THESE)
-- =========================
local SONG2_SLOT_ID = "Checkpoint2"
local SONG2_LENGTH_SECONDS = 1.0

-- Example CP2 targets (change/remove as needed)
-- local CP2_HIDE_ENTITY_ID = 0
-- local CP2_SHOW_ENTITY_ID = 0

function on_create(Self)
    Self.state = STATE_IDLE

    -- timers
    Self.timer = 0.0

    -- CP1 flags
    Self.cp1Started = false
    Self.cp1DoorDone = false
    Self.cp1ScatterDone = false

    -- CP2 flags
    Self.cp2Started = false
    Self.cp2Done = false

    if DEBUG then
        print("[StateController] on_create EntityID:", EntityID, "state=IDLE")
    end

    SHARED.StateController = SHARED.StateController or {}
    SHARED.StateController._self = Self

    function SHARED.StateController.TriggerCP1()
        local s = SHARED.StateController._self
        if not s then return end

        s.state = STATE_CP1
        s.timer = 0.0

        s.cp1Started = false
        s.cp1DoorDone = false
        s.cp1ScatterDone = false

        if DEBUG then
            print("[StateController] TriggerCP1 -> state=CP1")
        end
    end

    function SHARED.StateController.TriggerCP2()
        local s = SHARED.StateController._self
        if not s then return end

        s.state = STATE_CP2
        s.timer = 0.0

        s.cp2Started = false
        s.cp2Done = false

        if DEBUG then
            print("[StateController] TriggerCP2 -> state=CP2")
        end
    end
end

-- =========================
-- CP1
-- =========================
local function Checkpoint1(Self, dt)
    -- Start CP1 once
    if not Self.cp1Started then
        Self.cp1Started = true
        Self.timer = 0.0

        if DEBUG then
            print("[StateController] CP1: playing slot:", SONG1_SLOT_ID)
        end

        Audio_PlaySlot(EntityID, SONG1_SLOT_ID)
    end

    -- Count time
    Self.timer = Self.timer + dt

    -- After duration: shrink hidden door + scatter chair once
    if Self.timer >= SONG1_LENGTH_SECONDS then
        if not Self.cp1DoorDone then
            Self.cp1DoorDone = true

            local tr = Query.GetTransform(HIDDEN_DOOR_ID)
            if tr then
                tr:SetScale(0.0, 0.0, 0.0)
                if DEBUG then
                    print("[StateController] CP1 -> shrunk hidden door id:", HIDDEN_DOOR_ID)
                end
            else
                print("[StateController] CP1 ERROR: No Transform for hidden door id:", HIDDEN_DOOR_ID)
            end
        end

        if not Self.cp1ScatterDone then
            Self.cp1ScatterDone = true
            if SHARED.ChairScatter and SHARED.ChairScatter.Trigger then
                SHARED.ChairScatter.Trigger()
                if DEBUG then
                    print("[StateController] CP1 -> triggered ChairScatter")
                end
            else
                print("[StateController] CP1 WARN: ChairScatter not found (attach ChairScatter_CP1.lua somewhere)")
            end
        end
    end
end

-- =========================
-- CP2
-- =========================
local function Checkpoint2(Self, dt)
    -- Start CP2 once
    if not Self.cp2Started then
        Self.cp2Started = true
        Self.timer = 0.0

        if DEBUG then
            print("[StateController] CP2: playing slot:", SONG2_SLOT_ID)
        end

        -- Original CP2 music
        Audio_PlaySlot(EntityID, SONG2_SLOT_ID)
    end

    -- Count time
    Self.timer = Self.timer + dt

    -- After duration: do CP2 actions once
    if (not Self.cp2Done) and (Self.timer >= SONG2_LENGTH_SECONDS) then
        Self.cp2Done = true

        ----------------------------------------------------------------
        -- 🔊 NEW: trigger extra CP2 audio from another script
        ----------------------------------------------------------------
        if SHARED.CP2Audio and SHARED.CP2Audio.Play then
            SHARED.CP2Audio.Play()
        else
            print("[StateController] CP2 WARN: CP2Audio.Play not found")
        end

        ----------------------------------------------------------------
        -- Put other CP2 actions here if needed
        ----------------------------------------------------------------
        -- local tr = Query.GetTransform(123)
        -- if tr then tr:SetScale(0,0,0) end

        if DEBUG then
            print("[StateController] CP2 -> actions complete")
        end
    end
end


function on_update(Self, dt)
    if Self.state == STATE_IDLE then
        return
    end

    dt = dt or 0.016

    if Self.state == STATE_CP1 then
        Checkpoint1(Self, dt)
    elseif Self.state == STATE_CP2 then
        Checkpoint2(Self, dt)
    end
end

function on_destroy(Self)
    if SHARED.StateController and SHARED.StateController._self == Self then
        SHARED.StateController._self = nil
    end
end
