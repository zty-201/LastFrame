local DEBUG = true

local CUTSCENE_SCENE_SECONDS = 5.0
local TOTAL_SCENES = 5
local CUTSCENE_TOTAL_SECONDS = CUTSCENE_SCENE_SECONDS * TOTAL_SCENES

local lastSceneIndex = 0
local cutsceneActive = false
local cutsceneTimer = 0.0

local playedCueKeys = {}
local activeCutsceneSlots = {}

local CUTSCENE_AUDIO_CUES = {
    { scene = 1, time = 0.0, slot = "S1" },
    { scene = 2, time = 0.0, slot = "S2" },
    { scene = 3, time = 0.0, slot = "S3" },
    { scene = 4, time = 0.0, slot = "S4" },
    { scene = 5, time = 0.0, slot = "S5" },
}

local function play_slot(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        print("[SFX_CutsceneAudio] TRY PLAY slot: " .. tostring(slotId) .. " on EntityID=" .. tostring(EntityID))
        Audio_PlaySlot(EntityID, slotId)
        activeCutsceneSlots[slotId] = true
        if DEBUG then
            print("[SFX_CutsceneAudio] Played slot: " .. tostring(slotId))
        end
    else
        print("[SFX_CutsceneAudio] ERROR: Audio_PlaySlot missing or invalid slot " .. tostring(slotId))
    end
end

local function stop_all_cutscene_slots()
    if type(Audio_StopSlot) ~= "function" then
        return
    end

    for slotId, _ in pairs(activeCutsceneSlots) do
        Audio_StopSlot(EntityID, slotId)
        if DEBUG then
            print("[SFX_CutsceneAudio] Stopped slot: " .. tostring(slotId))
        end
    end

    activeCutsceneSlots = {}
end

local function reset_cutscene_audio_state()
    cutsceneTimer = 0.0
    playedCueKeys = {}
    lastSceneIndex = 0
    stop_all_cutscene_slots()
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CUTSCENE_AUDIO_COMMAND = nil

    cutsceneActive = false
    cutsceneTimer = 0.0
    playedCueKeys = {}
    activeCutsceneSlots = {}
    lastSceneIndex = 0

    if DEBUG then
        print("[SFX_CutsceneAudio] Ready on entity " .. tostring(EntityID))
    end
end

function on_update(self, dt)
    dt = dt or 0.016
    SHARED = SHARED or {}

    if SHARED.CUTSCENE_AUDIO_COMMAND == "START" then
        SHARED.CUTSCENE_AUDIO_COMMAND = nil
        cutsceneActive = true
        reset_cutscene_audio_state()

        SHARED.CUTSCENE_AMBIENCE = "INDOOR"
        SHARED.GAMEPLAY_BASE_AMBIENCE = "OUTDOOR"

        if DEBUG then
            print("[SFX_CutsceneAudio] Received START command from AmbienceManager")
            print("[SFX_CutsceneAudio] Initial cutscene ambience set to INDOOR")
            print("[SFX_CutsceneAudio] Gameplay ambience preset to OUTDOOR")
        end
    elseif SHARED.CUTSCENE_AUDIO_COMMAND == "STOP" then
        SHARED.CUTSCENE_AUDIO_COMMAND = nil

        if DEBUG then
            print("[SFX_CutsceneAudio] Received STOP command from AmbienceManager")
        end

        cutsceneActive = false
        reset_cutscene_audio_state()
        return
    end

    if not cutsceneActive then
        return
    end

    if DEBUG then
        print("[SFX_CutsceneAudio] Active timer=" .. tostring(cutsceneTimer))
    end

    cutsceneTimer = cutsceneTimer + dt

    local sceneIndex = math.floor(cutsceneTimer / CUTSCENE_SCENE_SECONDS) + 1
    if sceneIndex < 1 then sceneIndex = 1 end
    if sceneIndex > TOTAL_SCENES then sceneIndex = TOTAL_SCENES end

    if sceneIndex ~= lastSceneIndex then
        lastSceneIndex = sceneIndex
        print("[SFX_CutsceneAudio] Scene changed to " .. tostring(sceneIndex))

        if sceneIndex == 4 then
            SHARED.CUTSCENE_AMBIENCE = "OUTDOOR"
            SHARED.GAMEPLAY_BASE_AMBIENCE = "OUTDOOR"
            print("[SFX_CutsceneAudio] Requested ambience OUTDOOR")
            print("[SFX_CutsceneAudio] Gameplay ambience set to OUTDOOR")
        end
    end

    local sceneLocalTime = cutsceneTimer - ((sceneIndex - 1) * CUTSCENE_SCENE_SECONDS)

    for i = 1, #CUTSCENE_AUDIO_CUES do
        local cue = CUTSCENE_AUDIO_CUES[i]
        local cueKey = tostring(cue.scene) .. "_" .. tostring(cue.time) .. "_" .. tostring(cue.slot)

        if not playedCueKeys[cueKey] and cue.scene == sceneIndex and sceneLocalTime >= cue.time then
            play_slot(cue.slot)
            playedCueKeys[cueKey] = true
        end
    end

    if cutsceneTimer >= CUTSCENE_TOTAL_SECONDS then
        if DEBUG then
            print("[SFX_CutsceneAudio] Timer finished")
        end
        cutsceneActive = false
        reset_cutscene_audio_state()
    end
end

function on_destroy(self)
    stop_all_cutscene_slots()
    cutsceneActive = false
end