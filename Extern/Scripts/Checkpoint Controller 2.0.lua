----------------------------------------------------------------
-- StateController_CP1_PlaySong1.lua  (UPDATED: scatter chair + drop entity 522)
-- CP1: Play audio once, wait, then shrink hidden door + scatter chair parts
----------------------------------------------------------------

local DEBUG = true

local STATE_IDLE = 0
local STATE_CP1  = 1

local SONG_SLOT_ID = "Checkpoint1"
local SONG_LENGTH_SECONDS = 10.0

local HIDDEN_DOOR_ID = 43

function on_create(Self)
    Self.state = STATE_IDLE
    Self.timer = 0.0
    Self.cp1Started = false
    Self.hiddenDoorDone = false
    Self.scatterDone = false   -- ✅ NEW

    if DEBUG then
        print("[StateController] on_create EntityID:", EntityID, "state=IDLE")
    end

    SHARED.StateController = SHARED.StateController or {}
    SHARED.StateController._self = Self

    function SHARED.StateController.TriggerCP1()
        local s = SHARED.StateController._self
        if not s then return end

        if s.state == STATE_IDLE then
            s.state = STATE_CP1
            s.timer = 0.0
            s.cp1Started = false
            s.hiddenDoorDone = false
            s.scatterDone = false -- ✅ reset

            if DEBUG then
                print("[StateController] TriggerCP1 -> state=CP1")
            end
        end
    end
end

local function Checkpoint1(Self, dt)
    -- 1️⃣ Start CP1 once
    if not Self.cp1Started then
        Self.cp1Started = true
        Self.timer = 0.0

        if DEBUG then
            print("[StateController] CP1: playing slot:", SONG_SLOT_ID)
        end

        Audio_PlaySlot(EntityID, SONG_SLOT_ID)
    end

    -- 2️⃣ Count time
    Self.timer = Self.timer + dt

    -- 3️⃣ After song duration, shrink hidden door once + scatter chair once
    if (not Self.hiddenDoorDone) and (Self.timer >= SONG_LENGTH_SECONDS) then
        Self.hiddenDoorDone = true

        local tr = Query.GetTransform(HIDDEN_DOOR_ID)
        if tr then
            tr:SetScale(0.0, 0.0, 0.0)
            if DEBUG then
                print("[StateController] CP1 -> shrunk hidden door id:", HIDDEN_DOOR_ID)
            end
        else
            print("[StateController] CP1 ERROR: No Transform for hidden door id:", HIDDEN_DOOR_ID)
        end

        -- ✅ NEW: trigger chair scatter (if ChairScatter script exists)
        if (not Self.scatterDone) then
            Self.scatterDone = true

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

function on_update(Self, dt)
    if Self.state == STATE_IDLE then
        return
    end

    if Self.state == STATE_CP1 then
        Checkpoint1(Self, dt)
    end
end

function on_destroy(Self)
    if SHARED.StateController and SHARED.StateController._self == Self then
        SHARED.StateController._self = nil
    end
end
