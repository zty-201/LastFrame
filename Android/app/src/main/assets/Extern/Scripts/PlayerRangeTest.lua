-- PlayerRangeTest.lua
-- Attach to player. Change TARGET_ID to some other entity's ID.

local TARGET_ID = 62         -- TODO: change this to the other entity's ID
local RANGE     = 150.0         -- test radius (world units)

local wasInRange = false
local timeAcc = 0.0

function on_create(self)
    print("[RangeTest] Loaded on EntityID =", EntityID)
    print("[RangeTest] Testing against TARGET_ID =", TARGET_ID, "RANGE =", RANGE)
end

function on_update(self, dt)
    -- throttle checks so logs are readable (5 times per second)
    timeAcc = timeAcc + dt
    if timeAcc < 0.2 then
        return
    end
    timeAcc = 0.0

    local inRange = Query.IsEntityInRange(EntityID, TARGET_ID, RANGE)

    if inRange and not wasInRange then
        print("[RangeTest] ENTER range of", TARGET_ID)
    elseif (not inRange) and wasInRange then
        print("[RangeTest] EXIT range of", TARGET_ID)
    end

    wasInRange = inRange
end

function on_destroy(self)
    print("[RangeTest] Destroyed for EntityID =", EntityID)
end
