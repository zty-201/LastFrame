-- InteractionBox_FollowPlayer.lua
-- Attach this to the invisible interaction box entity

-- ========= CONFIG =========
local PLAYER_ID = 5          -- <-- CHANGE if needed
local OFFSET_FORWARD = 30    -- distance in front of player
local OFFSET_Y = 0.8         -- height offset
local LOG_INTERVAL = 60      -- frames (roughly once per second)

-- ========= INTERNAL =========
local frame = 0
local printedFirstUpdate = false

-- helper: safe tostring
local function s(v)
    if v == nil then return "nil" end
    return tostring(v)
end

-- helper: safe call (won't crash if function doesn't exist)
local function safe_call(label, fn, ...)
    if fn == nil then
        return false, nil
    end
    local ok, result = pcall(fn, ...)
    if not ok then
        print("[InteractionBox][SAFE_CALL FAIL] " .. label .. " -> " .. s(result))
        return false, nil
    end
    return true, result
end

-- helper: try get entity name if your engine has it
local function try_get_name(eid)
    if Query and Query.GetName then
        local ok, name = safe_call("Query.GetName", Query.GetName, eid)
        if ok then return name end
    end
    return nil
end

function on_create(self)
    print("[InteractionBox] on_create")
    print("[InteractionBox] EntityID =", EntityID, " PLAYER_ID =", PLAYER_ID)

    -- Quick sanity checks (won't crash if missing)
    if Query == nil then
        print("[InteractionBox] WARNING: Query is nil (Lua bindings not available?)")
    else
        print("[InteractionBox] Query table OK")
    end

    -- If your engine supports printing components, try these safely:
    if Query then
        safe_call("Query.GetTransform(EntityID)", Query.GetTransform, EntityID)
        safe_call("Query.GetTransform(PLAYER_ID)", Query.GetTransform, PLAYER_ID)
    end
end

function on_update(self, dt)
    frame = frame + 1

    -- One-time log to prove on_update is firing + dt looks sane
    if not printedFirstUpdate then
        printedFirstUpdate = true
        print("[InteractionBox] on_update is RUNNING. dt =", s(dt))
    end

    -- Get transforms
    local playerTr = Query and Query.GetTransform and Query.GetTransform(PLAYER_ID) or nil
    local boxTr    = Query and Query.GetTransform and Query.GetTransform(EntityID) or nil

    if playerTr == nil then
        if frame % LOG_INTERVAL == 0 then
            print("[InteractionBox] ERROR: player transform is nil (PLAYER_ID =", PLAYER_ID, ")")
        end
        return
    end

    if boxTr == nil then
        if frame % LOG_INTERVAL == 0 then
            print("[InteractionBox] ERROR: box transform is nil (EntityID =", EntityID, ")")
        end
        return
    end

    -- Player position
    local px, py, pz = playerTr:GetPosition()

    -- Forward direction (camera-based)
    local fx, fy, fz = 0, 0, 1
    if Query and Query.GetCameraForward then
        fx, fy, fz = Query.GetCameraForward(PLAYER_ID)
    end

    -- Compute target position
    local tx = px + fx * OFFSET_FORWARD
    local ty = py + OFFSET_Y
    local tz = pz + fz * OFFSET_FORWARD

    -- Move the box
    boxTr:SetPosition(tx, ty, tz)

    -- Heartbeat logs (proves script is alive + shows motion)
    if frame % LOG_INTERVAL == 0 then
        print(string.format("[InteractionBox] PlayerPos = (%.2f, %.2f, %.2f)", px, py, pz))
        print(string.format("[InteractionBox] BoxPos    = (%.2f, %.2f, %.2f)", tx, ty, tz))
        print(string.format("[InteractionBox] Forward   = (%.2f, %.2f, %.2f)", fx, fy, fz))
    end

    -- ================================
    -- COLLISION DEBUG (polling fallback)
    -- ================================
    -- This tries a few likely engine APIs without crashing.
    -- If your engine supports NONE of these, you’ll still see SAFE_CALL FAIL logs
    -- telling you what exists / doesn't.

    if Query then
        -- 1) Common: Query.GetOverlappingEntities(entityId)
        if frame % LOG_INTERVAL == 0 and Query.GetOverlappingEntities then
            local ok, list = safe_call("Query.GetOverlappingEntities", Query.GetOverlappingEntities, EntityID)
            if ok and list then
                print("[InteractionBox] Overlaps count =", s(#list))
                for i = 1, #list do
                    local eid = list[i]
                    local nm = try_get_name(eid)
                    if nm then
                        print("  -> overlap with", eid, "(" .. nm .. ")")
                    else
                        print("  -> overlap with", eid)
                    end
                end
            end
        end

        -- 2) Common: Query.GetCollidingEntities(entityId)
        if frame % LOG_INTERVAL == 0 and Query.GetCollidingEntities then
            local ok, list = safe_call("Query.GetCollidingEntities", Query.GetCollidingEntities, EntityID)
            if ok and list then
                print("[InteractionBox] Collisions count =", s(#list))
                for i = 1, #list do
                    local eid = list[i]
                    local nm = try_get_name(eid)
                    if nm then
                        print("  -> colliding with", eid, "(" .. nm .. ")")
                    else
                        print("  -> colliding with", eid)
                    end
                end
            end
        end

        -- 3) If your engine exposes PhysicsWorld overlap checks (examples)
        -- These will just print SAFE_CALL FAIL if they don't exist.
        if frame % LOG_INTERVAL == 0 then
            safe_call("Query.OverlapBox (if exists)", Query.OverlapBox, EntityID)
            safe_call("PhysicsWorld.OverlapBox (if exists)", PhysicsWorld and PhysicsWorld.OverlapBox, EntityID)
        end
    end
end

-- ==========================================================
-- COLLISION/TRIGGER CALLBACKS (event-based, best if supported)
-- ==========================================================
-- Your engine might call ONE of these names. We implement many.
-- If any of them fire, you'll know collision events are hooked up.

local function log_hit(tag, other)
    local otherId = other
    -- some engines pass a table {entity=ID} or {id=ID}
    if type(other) == "table" then
        otherId = other.entity or other.id or other.EntityID or other[1]
    end

    local nm = try_get_name(otherId)
    if nm then
        print("[InteractionBox][" .. tag .. "] hit with", s(otherId), "(" .. nm .. ")")
    else
        print("[InteractionBox][" .. tag .. "] hit with", s(otherId))
    end
end

function on_trigger_enter(self, other)  log_hit("on_trigger_enter", other) end
function on_trigger_exit(self, other)   log_hit("on_trigger_exit", other) end
function on_trigger_stay(self, other)   log_hit("on_trigger_stay", other) end

function on_collision_enter(self, other) log_hit("on_collision_enter", other) end
function on_collision_exit(self, other)  log_hit("on_collision_exit", other) end
function on_collision_stay(self, other)  log_hit("on_collision_stay", other) end

-- alternative naming some engines use:
function on_begin_overlap(self, other)  log_hit("on_begin_overlap", other) end
function on_end_overlap(self, other)    log_hit("on_end_overlap", other) end
function on_overlap(self, other)        log_hit("on_overlap", other) end
