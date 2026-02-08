-- indoor_detector.lua
print("[IndoorAudio] script file loaded")

local PLAYER_ENTITY_ID = 5

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
    print(string.format("[IndoorAudio] Trigger ENTER | Zone=%d | Other=%d", EntityID, otherID))

    if otherID ~= PLAYER_ENTITY_ID then
        print(string.format("[IndoorAudio] ENTER ignored: not player (PlayerID=%d).", PLAYER_ENTITY_ID))
        return
    end

    if playerInsideThisZone then
        print("[IndoorAudio] Already inside this zone. Debounced.")
        return
    end
    playerInsideThisZone = true

    _G.INDOOR_COUNT = (_G.INDOOR_COUNT or 0) + 1
    print(string.format(">>> PLAYER ENTERED INDOOR ZONE %d | INDOOR_COUNT=%d", EntityID, _G.INDOOR_COUNT))
end

function on_trigger_exit(self, otherID)
    print(string.format("[IndoorAudio] Trigger EXIT | Zone=%d | Other=%d", EntityID, otherID))

    if otherID ~= PLAYER_ENTITY_ID then
        print(string.format("[IndoorAudio] EXIT ignored: not player (PlayerID=%d).", PLAYER_ENTITY_ID))
        return
    end

    if not playerInsideThisZone then
        print("[IndoorAudio] Exit ignored — player wasn't marked inside this zone.")
        return
    end
    playerInsideThisZone = false

    _G.INDOOR_COUNT = math.max(0, (_G.INDOOR_COUNT or 0) - 1)
    print(string.format("<<< PLAYER EXITED INDOOR ZONE %d | INDOOR_COUNT=%d", EntityID, _G.INDOOR_COUNT))
end

function on_destroy(self)
    -- Safety: if zone is destroyed while player is inside, fix global count
    if playerInsideThisZone then
        playerInsideThisZone = false
        _G.INDOOR_COUNT = math.max(0, (_G.INDOOR_COUNT or 0) - 1)
        print(string.format(
            "[IndoorAudio] on_destroy: zone removed while inside -> INDOOR_COUNT=%d",
            _G.INDOOR_COUNT
        ))
    end
end
