----------------------------------------------------------------
-- StateController_CP1_CP2.lua
-- CP1: TTLS_Start + Tension
-- CP2: Chair explode
-- CP3: TTLS_End
----------------------------------------------------------------

local DEBUG = true

local STATE_IDLE = 0
local STATE_CP1  = 1
local STATE_CP2  = 2
local STATE_CP3  = 3

-- =========================
-- CP1 CONFIG
-- =========================
local CP1_SLOT_ID = "TTLS_Start"
local CP1_TENSION_SLOT_ID = "Tension"
local CP1_LENGTH_SECONDS = 10.1

local CP1_VOICEOVER_SLOT_ID = "VoiceOver_02"
local CP1_VOICEOVER_DELAY_SECONDS = 6.5

-- =========================
-- CP2 CONFIG
-- =========================
local CP2_DELAY_SECONDS = 1.6
local HIDDEN_DOOR_ID = 43

local CP2_REACTION_SLOT_ID = "VoiceOver_03"
local CP2_REACTION_DELAY = 0.5
local CP2_TO_CP3_DELAY_AFTER_REACTION = 0.6

-- =========================
-- CP3 CONFIG
-- =========================
local CP3_SLOT_ID = "TTLS_End"
local CP3_LENGTH_SECONDS = 23.0

local function play_slot_from(entityId, slotId)
    if type(Audio_PlaySlot) == "function" and entityId ~= nil and slotId ~= nil then
        Audio_PlaySlot(entityId, slotId)
        return true
    end
    return false
end

local function stop_slot_from(entityId, slotId)
    if type(Audio_StopSlot) == "function" and entityId ~= nil and slotId ~= nil then
        Audio_StopSlot(entityId, slotId)
        return true
    end
    return false
end

function on_create(Self)
    Self.state = STATE_IDLE
    Self.timer = 0.0

    -- CP1 flags
    Self.cp1Started = false
    Self.cp1VoiceOverDone = false

    -- CP2 flags
    Self.cp2Started = false
    Self.cp2DoorDone = false
    Self.cp2ScatterDone = false
    Self.cp2ChairAudioDone = false
    Self.cp2ReactionVoiceDone = false
    Self.cp2ReactionVoiceTimer = 0.0
    Self.cp2ReadyForCP3 = false

    -- CP3 flags
    Self.cp3Started = false

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
        s.cp1VoiceOverDone = false
        s.cp2Started = false
        s.cp2DoorDone = false
        s.cp2ScatterDone = false
        s.cp2ChairAudioDone = false
        s.cp2ReactionVoiceDone = false
        s.cp2ReactionVoiceTimer = 0.0
        s.cp2ReadyForCP3 = false
        s.cp3Started = false

        if DEBUG then
            print("[StateController] TriggerCP1 -> state=CP1")
        end
    end

    function SHARED.StateController.TriggerCP2()
        local s = SHARED.StateController._self
        if not s then return end

        stop_slot_from(EntityID, CP1_TENSION_SLOT_ID)

        s.state = STATE_CP2
        s.timer = 0.0

        s.cp2Started = false
        s.cp2DoorDone = false
        s.cp2ScatterDone = false
        s.cp2ChairAudioDone = false
        s.cp2ReactionVoiceDone = false
        s.cp2ReactionVoiceTimer = 0.0
        s.cp2ReadyForCP3 = false

        if DEBUG then
            print("[StateController] TriggerCP2 -> state=CP2")
        end
    end

    function SHARED.StateController.TriggerCP3()
        local s = SHARED.StateController._self
        if not s then return end

        s.state = STATE_CP3
        s.timer = 0.0

        s.cp3Started = false

        if DEBUG then
            print("[StateController] TriggerCP3 -> state=CP3")
        end
    end
end

-- =========================
-- CP1 = TTLS_Start + Tension
-- =========================
local function Checkpoint1(Self, dt)
    if not Self.cp1Started then
        Self.cp1Started = true
        Self.timer = 0.0
        Self.cp1VoiceOverDone = false

        if DEBUG then
            print("[StateController] CP1: playing slots:", CP1_SLOT_ID, "and", CP1_TENSION_SLOT_ID)
        end

        play_slot_from(EntityID, CP1_SLOT_ID)
        play_slot_from(EntityID, CP1_TENSION_SLOT_ID)
    end

    Self.timer = Self.timer + dt

    ------------------------------------------------
    -- Play VoiceOver_02 later in CP1, after the girl-run portion
    ------------------------------------------------
    if (not Self.cp1VoiceOverDone) and Self.timer >= CP1_VOICEOVER_DELAY_SECONDS then
        Self.cp1VoiceOverDone = true
        play_slot_from(EntityID, CP1_VOICEOVER_SLOT_ID)

        SHARED.SUBTITLE_CMD = SHARED.SUBTITLE_CMD or {}
        SHARED.SUBTITLE_CMD["CP1_DETECTIVE_REACTION"] = {
            trigger = true,
            by = EntityID
        }

        if DEBUG then
            print("[StateController] CP1 -> played delayed voiceover + subtitle:", CP1_VOICEOVER_SLOT_ID)
        end
    end

    if Self.timer >= CP1_LENGTH_SECONDS then
        if DEBUG then
            print("[StateController] CP1 complete -> triggering CP2")
        end

        SHARED.StateController.TriggerCP2()
    end
end

-- =========================
-- CP2 = Chair explode
-- =========================
local function Checkpoint2(Self, dt)
    if not Self.cp2Started then
        Self.cp2Started = true
        Self.timer = 0.0
        Self.cp2ReactionVoiceTimer = 0.0
        Self.cp2ReactionVoiceDone = false
        Self.cp2ReadyForCP3 = false

        if DEBUG then
            print("[StateController] CP2: waiting", CP2_DELAY_SECONDS, "seconds before chair explode")
        end
    end

    Self.timer = Self.timer + dt

    ------------------------------------------------
    -- Explosion moment
    ------------------------------------------------
    if Self.timer >= CP2_DELAY_SECONDS then
        if not Self.cp2DoorDone then
            Self.cp2DoorDone = true

            local tr = Query.GetTransform(HIDDEN_DOOR_ID)
            if tr then
                tr:SetScale(0.0, 0.0, 0.0)
                if DEBUG then
                    print("[StateController] CP2 -> shrunk hidden door id:", HIDDEN_DOOR_ID)
                end
            else
                print("[StateController] CP2 ERROR: No Transform for hidden door id:", HIDDEN_DOOR_ID)
            end
        end

        if not Self.cp2ScatterDone then
            Self.cp2ScatterDone = true

            if SHARED.ChairScatter and SHARED.ChairScatter.Trigger then
                SHARED.ChairScatter.Trigger()
                if DEBUG then
                    print("[StateController] CP2 -> triggered ChairScatter")
                end
            else
                print("[StateController] CP2 WARN: ChairScatter not found")
            end

            SHARED = SHARED or {}
            SHARED.POV = SHARED.POV or {}
            SHARED.POV.freeShake      = true
            SHARED.POV.shakeDuration  = 0.7
            SHARED.POV.shakeAmplitude = 10.0
            SHARED.POV.shakeSpeed     = 0.03

            Self.cp2ReactionVoiceTimer = 0.0
            Self.cp2ReadyForCP3 = false

            if DEBUG then
                print("[StateController] CP2 -> triggered free camera shake")
            end
        end
    end

    ------------------------------------------------
    -- Delayed Woah! reaction after explosion
    ------------------------------------------------
    if Self.cp2ScatterDone and (not Self.cp2ReactionVoiceDone) then
        Self.cp2ReactionVoiceTimer = Self.cp2ReactionVoiceTimer + dt

        if Self.cp2ReactionVoiceTimer >= CP2_REACTION_DELAY then
            Self.cp2ReactionVoiceDone = true
            play_slot_from(EntityID, CP2_REACTION_SLOT_ID)

            SHARED.SUBTITLE_CMD = SHARED.SUBTITLE_CMD or {}
            SHARED.SUBTITLE_CMD["CP2_WOAH"] = {
                trigger = true,
                by = EntityID
            }

            if DEBUG then
                print("[StateController] CP2 -> Woah! voice + subtitle")
            end
        end
    end

    ------------------------------------------------
    -- Wait a little after the reaction before moving to CP3
    ------------------------------------------------
    if Self.cp2ScatterDone and Self.cp2ReactionVoiceDone and (not Self.cp2ReadyForCP3) then
        if Self.cp2ReactionVoiceTimer >= (CP2_REACTION_DELAY + CP2_TO_CP3_DELAY_AFTER_REACTION) then
            Self.cp2ReadyForCP3 = true
        end
    end

    if Self.cp2ReadyForCP3 then
        if DEBUG then
            print("[StateController] CP2 complete -> triggering CP3")
        end

        SHARED.StateController.TriggerCP3()
    end
end

-- =========================
-- CP3 = TTLS_End
-- =========================
local function Checkpoint3(Self, dt)
    if not Self.cp3Started then
        Self.cp3Started = true
        Self.timer = 0.0

        if DEBUG then
            print("[StateController] CP3: playing slot:", CP3_SLOT_ID)
        end

        play_slot_from(EntityID, CP3_SLOT_ID)
    end

    Self.timer = Self.timer + dt

    if Self.timer >= CP3_LENGTH_SECONDS then
        if DEBUG then
            print("[StateController] CP3 complete -> state=IDLE")
        end

        Self.state = STATE_IDLE
        Self.timer = 0.0
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
    elseif Self.state == STATE_CP3 then
        Checkpoint3(Self, dt)
    end
end

function on_destroy(Self)
    stop_slot_from(EntityID, CP1_TENSION_SLOT_ID)

    if SHARED.StateController and SHARED.StateController._self == Self then
        SHARED.StateController._self = nil
    end
end