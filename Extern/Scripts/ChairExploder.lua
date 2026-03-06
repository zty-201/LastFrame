----------------------------------------------------------------
-- ChairScatter_CP1.lua (SAFE ZONE VERSION)
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

-- Safe zone: pieces will land randomly within this radius of the center
local ZONE_CENTER_X  = 443.983
local ZONE_CENTER_Z  = -288.646
local ZONE_RADIUS    = 150.0    -- tune this if pieces still hit walls

-- Y settings
local BURST_Y        = 80.0    -- upward kick on explosion
local GRAVITY        = 280.0   -- how fast pieces fall
local Y_FLOOR        = 5.0     -- height pieces stop at (tune to your floor)
local Y_MAX          = 100.0   -- max height a piece can reach

local function log(...)
    if DEBUG then print("[ChairScatter]", ...) end
end

-- Pick a random point within the safe zone circle
local function random_landing()
    local angle  = math.random() * math.pi * 2.0
    local radius = math.random() * ZONE_RADIUS
    local lx = ZONE_CENTER_X + math.cos(angle) * radius
    local lz = ZONE_CENTER_Z + math.sin(angle) * radius
    return lx, lz
end

function on_create(Self)
    Self.done       = false
    Self.simulating = false
    Self.pieces     = {}

    SHARED.ChairScatter = SHARED.ChairScatter or {}
    SHARED.ChairScatter._self = Self

    function SHARED.ChairScatter.Trigger()
        local s = SHARED.ChairScatter._self
        if not s or s.done then return end
        s.done = true

        log("Explosion triggered!")

        ----------------------------------------------------------------
        -- 1. For each piece, pick a target landing spot in the safe zone
        --    then compute a velocity that arcs it toward that spot
        ----------------------------------------------------------------
        s.pieces = {}
        for _, id in ipairs(CHAIR_PART_IDS) do
            local tr = Query.GetTransform(id)
            if tr then
                local x, y, z = tr:GetPosition()

                -- Pick a random landing spot inside the safe zone
                local tx, tz = random_landing()

                -- Estimate flight time based on upward burst + gravity
                local vy = math.random() * BURST_Y * 0.5 + BURST_Y * 0.5
                local t  = (2.0 * vy) / GRAVITY
                if t < 0.1 then t = 0.1 end

                -- Work out horizontal velocity needed to reach target in time t
                local vx = (tx - x) / t
                local vz = (tz - z) / t

                table.insert(s.pieces, {
                    id      = id,
                    vx      = vx,
                    vy      = vy,
                    vz      = vz,
                    stopped = false
                })
            else
                log("WARN: no Transform for chair part:", id)
            end
        end

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

            -- Apply gravity
            p.vy = p.vy - GRAVITY * dt

            local tr = Query.GetTransform(p.id)
            if tr then
                local x, y, z = tr:GetPosition()

                local nx = x + p.vx * dt
                local ny = y + p.vy * dt
                local nz = z + p.vz * dt

                -- Clamp max height
                if ny > Y_MAX then
                    ny = Y_MAX
                    p.vy = 0.0
                end

                -- Hit the floor, stop the piece
                if ny <= Y_FLOOR then
                    ny = Y_FLOOR
                    p.vy = 0.0
                    p.vx = 0.0
                    p.vz = 0.0
                    p.stopped = true
                    log("Piece", p.id, "landed")
                end

                tr:SetPosition(nx, ny, nz)
            end
        end
    end

    if allStopped then
        Self.simulating = false
        log("All pieces landed, simulation done")
    end
end

function on_destroy(Self)
    if SHARED.ChairScatter and SHARED.ChairScatter._self == Self then
        SHARED.ChairScatter._self = nil
    end
end