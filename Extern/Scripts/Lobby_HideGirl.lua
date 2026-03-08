----------------------------------------------------------------
-- Lobby_HideGirl.lua
-- Attach to the little girl entity in the lobby.
-- Hides the girl shortly after the canteen door slams shut.
----------------------------------------------------------------

local DEBUG = true

local CANTEEN_DOOR_ID = 400

local hasHidden = false

-- delay so she disappears AFTER the slam finishes
local HIDE_DELAY = 1.0
local hideTimer = 0.0
local waitingToHide = false

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CANTEEN_DOOR_LOCKED = SHARED.CANTEEN_DOOR_LOCKED or {}

    hasHidden = false
    hideTimer = 0.0
    waitingToHide = false

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[Lobby_HideGirl] ERROR: No Transform found for EntityID=" .. tostring(EntityID))
        return
    end

    if DEBUG then
        print(string.format(
            "[Lobby_HideGirl] Ready | GirlID=%d | Watching DoorID=%d",
            EntityID, CANTEEN_DOOR_ID
        ))
    end
end

function on_update(self, dt)

    if hasHidden then return end
    dt = dt or 0.016

    if SHARED == nil or SHARED.CANTEEN_DOOR_LOCKED == nil then
        return
    end

    -- Start delay once door locks
    if SHARED.CANTEEN_DOOR_LOCKED[CANTEEN_DOOR_ID] == true and not waitingToHide then
        waitingToHide = true
        hideTimer = 0.0

        if DEBUG then
            print("[Lobby_HideGirl] Door locked -> starting hide timer")
        end
    end

    -- Run delay timer
    if waitingToHide then
        hideTimer = hideTimer + dt

        if hideTimer >= HIDE_DELAY then
            local tr = Query.GetTransform(EntityID)
            if tr == nil then return end

            tr:SetScale(0.0, 0.0, 0.0)
            hasHidden = true

            if DEBUG then
                print(string.format(
                    "[Lobby_HideGirl] Girl hidden after delay | GirlID=%d",
                    EntityID
                ))
            end
        end
    end
end

function on_destroy(self)
end