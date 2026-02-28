----------------------------------------------------------------
-- ChairScatter_CP1.lua (EXPLOSION VERSION)
----------------------------------------------------------------
local DEBUG = true

-- Chair piece entity IDs
local CHAIR_PART_IDS = {
    201, 510, 511, 512, 513, 514, 515,
    516, 517, 518, 519, 520, 521
}

-- Special entities
local DROP_ENTITY_ID = 522
local HIDE_ENTITY_ID = 209
local SCALE_378_ID   = 378
local SCALE_380_ID   = 380

-- Explosion force ranges
local BURST_XZ   = 400.0   -- horizontal kick
local BURST_Y    = 350.0   -- upward kick
local GRAVITY    = 280.0   -- pull down per second squared

-- Y floor: pieces stop here
local Y_FLOOR    = 40.0

local function log(...)
    if DEBUG then print("[ChairScatter]", ...) end
end

local function rand_range(r)
    return (math.random() * 2.0 - 1.0) * r
end

function on_create(Self)
    Self.done       = false
    Self.simulating = false

    -- Will hold per-piece state: { id, vx, vy, vz, stopped }
    Self.pieces = {}

    SHARED.ChairScatter = SHARED.ChairScatter or {}
    SHARED.ChairScatter._self = Self

    function SHARED.ChairScatter.Trigger()
        local s = SHARED.ChairScatter._self
        if not s or s.done then return end
        s.done = true

        log("Explosion triggered!")

        ----------------------------------------------------------------
        -- 1. Assign random burst velocities to each chair piece
        ----------------------------------------------------------------
        s.pieces = {}
        for _, id in ipairs(CHAIR_PART_IDS) do
            local tr = Query.GetTransform(id)
            if tr then
                table.insert(s.pieces, {
                    id      = id,
                    vx      = rand_range(BURST_XZ),
                    vy      = math.random() * BURST_Y + 100.0,  -- always pops upward
                    vz      = rand_range(BURST_XZ),
                    stopped = false
                })
            else
                log("WARN: no Transform for chair part:", id)
            end
        end

        -- Start simulating in on_update
        s.simulating = true

        ----------------------------------------------------------------
        -- 2. Drop entity 522
        ----------------------------------------------------------------
        local trDrop = Query.GetTransform(DROP_ENTITY_ID)
        if trDrop then
            local x, _, z = trDrop:GetPosition()
            trDrop:SetPosition(x, -1000.0, z)
            log("Dropped entity", DROP_ENTITY_ID)
        end

        ----------------------------------------------------------------
        -- 3. Hide entity 209
        ----------------------------------------------------------------
        local trHide = Query.GetTransform(HIDE_ENTITY_ID)
        if trHide then
            trHide:SetScale(0.0, 0.0, 0.0)
            log("Hid entity", HIDE_ENTITY_ID)
        end

        ----------------------------------------------------------------
        -- 4. Scale entity 378 to 0.6
        ----------------------------------------------------------------
        local tr378 = Query.GetTransform(SCALE_378_ID)
        if tr378 then
            tr378:SetScale(0.6, 0.6, 0.6)
            log("Scaled entity", SCALE_378_ID, "to 0.6")
        end

        ----------------------------------------------------------------
        -- 5. Scale entity 380 to 1.0
        ----------------------------------------------------------------
        local tr380 = Query.GetTransform(SCALE_380_ID)
        if tr380 then
            tr380:SetScale(1.0, 1.0, 1.0)
            log("Scaled entity", SCALE_380_ID, "to 1.0")
        end
    end

    log("Ready. EntityID:", EntityID)
end

function on_update(Self, dt)
    if not Self.simulating then return end

    dt = dt or 0.016

    local allStopped = true

    for _, p in ipairs(Self.pieces) do
        if not p.stopped then
            allStopped = false

            -- Apply gravity to vertical velocity
            p.vy = p.vy - GRAVITY * dt

            -- Get current position
            local tr = Query.GetTransform(p.id)
            if tr then
                local x, y, z = tr:GetPosition()

                local nx = x + p.vx * dt
                local ny = y + p.vy * dt
                local nz = z + p.vz * dt

                -- Hit the floor?
                if ny <= Y_FLOOR then
                    ny = Y_FLOOR
                    p.vy = 0.0
                    p.vx = 0.0
                    p.vz = 0.0
                    p.stopped = true
                    log("Piece", p.id, "hit floor")
                end

                tr:SetPosition(nx, ny, nz)
            end
        end
    end

    -- All pieces landed, stop simulating
    if allStopped then
        Self.simulating = false
        log("All pieces settled, simulation done")
    end
end

function on_destroy(Self)
    if SHARED.ChairScatter and SHARED.ChairScatter._self == Self then
        SHARED.ChairScatter._self = nil
    end
end