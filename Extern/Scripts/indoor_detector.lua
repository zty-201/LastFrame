-- indoor_detector.lua
print("[IndoorAudio] script file loaded")

-- Mark which TriggerBox entity IDs count as indoor
local indoorZones = {
    [259] = true
}

local outdoorZones = {
    [377] = true
}

-- Shared across ALL zone scripts
if _G.INDOOR_COUNT == nil then
    _G.INDOOR_COUNT = 0
end

-- Local to THIS zone script instance (prevents double enter/exit)
local playerInsideThisZone = false

function on_create(self)
    print(string.format("[IndoorAudio] on_create | Zone=%d | PlayerID=%s",
        EntityID,
        tostring(SHARED.PlayerID)
    ))
    -- Do nothing else. Ambience manager will read _G.INDOOR_COUNT.
end

function on_trigger_enter(self, otherID)

    print(string.format("[IndoorAudio] Trigger ENTER fired | Zone=%d | Other=%d", EntityID, otherID))

    -- Lazy bind if needed
    if SHARED.PlayerID == nil then
        SHARED.PlayerID = otherID
        print(string.format("[IndoorAudio] PlayerID was nil -> bound PlayerID=%d (lazy bind)", SHARED.PlayerID))
    end

    -- Ensure it's the player
    if otherID ~= SHARED.PlayerID then
        print(string.format("[IndoorAudio] Not player (PlayerID=%d), ignoring.", SHARED.PlayerID))
        return
    end

    -- OUTDOOR OVERRIDE ZONE (607)
    if outdoorZones[EntityID] then
        _G.INDOOR_COUNT = 0
        print(string.format(">>> PLAYER ENTERED OUTDOOR OVERRIDE ZONE %d | INDOOR_COUNT=%d", EntityID, _G.INDOOR_COUNT))
        return
    end

    -- Only indoor zones affect the counter
    if not indoorZones[EntityID] then
        print("[IndoorAudio] Zone not registered as indoor. Ignoring.")
        return
    end

    -- Debounce
    if playerInsideThisZone then
        print("[IndoorAudio] Already inside this zone. Debounced.")
        return
    end
    playerInsideThisZone = true

    _G.INDOOR_COUNT = _G.INDOOR_COUNT + 1

    print(string.format(
        ">>> PLAYER ENTERED INDOOR ZONE %d | INDOOR_COUNT=%d",
        EntityID,
        _G.INDOOR_COUNT
    ))
end

function on_trigger_exit(self, otherID)

    print(string.format("[IndoorAudio] Trigger EXIT fired | Zone=%d | Other=%d", EntityID, otherID))

    if not indoorZones[EntityID] then
        return
    end

    if SHARED.PlayerID == nil then
        print("[IndoorAudio] EXIT ignored: PlayerID still nil.")
        return
    end

    if otherID ~= SHARED.PlayerID then
        return
    end

    -- Debounce
    if not playerInsideThisZone then
        print("[IndoorAudio] Exit ignored — player wasn't marked inside this zone.")
        return
    end
    playerInsideThisZone = false

    _G.INDOOR_COUNT = math.max(0, _G.INDOOR_COUNT - 1)

    print(string.format(
        "<<< PLAYER EXITED INDOOR ZONE %d | INDOOR_COUNT=%d",
        EntityID,
        _G.INDOOR_COUNT
    ))
end
