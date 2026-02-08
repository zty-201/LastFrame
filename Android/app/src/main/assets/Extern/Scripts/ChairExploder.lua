----------------------------------------------------------------
-- ChairScatter_CP1.lua (FINAL FINAL)
----------------------------------------------------------------
local DEBUG = true

-- Chair piece entity IDs
local CHAIR_PART_IDS = {
    201, 510, 511, 512, 513, 514, 515,
    516, 517, 518, 519, 520, 521
}

-- Special entities
local DROP_ENTITY_ID   = 522   -- send to y = -1000
local HIDE_ENTITY_ID   = 209   -- scale to 0,0,0
local SCALE_378_ID     = 378   -- scale to 0.6
local SCALE_380_ID     = 380   -- scale to 1.0

-- Scatter ranges
local SCATTER_RANGE_XZ = 300.0
local SCATTER_RANGE_Y  = 10.0

-- Optional Y clamp
local CLAMP_Y = true
local Y_MIN =  50.0
local Y_MAX =  150.0

local function log(...)
    if DEBUG then print("[ChairScatter]", ...) end
end

local function rand_range(r)
    return (math.random() * 2.0 - 1.0) * r
end

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function on_create(Self)
    Self.done = false

    SHARED.ChairScatter = SHARED.ChairScatter or {}
    SHARED.ChairScatter._self = Self

    function SHARED.ChairScatter.Trigger()
        local s = SHARED.ChairScatter._self
        if not s or s.done then return end
        s.done = true

        log("Triggering CP1 scatter effects")

        ----------------------------------------------------------------
        -- 1️⃣ Scatter chair parts
        ----------------------------------------------------------------
        for _, id in ipairs(CHAIR_PART_IDS) do
            local tr = Query.GetTransform(id)
            if tr then
                local x, y, z = tr:GetPosition()

                local nx = x + rand_range(SCATTER_RANGE_XZ)
                local ny = y + rand_range(SCATTER_RANGE_Y)
                local nz = z + rand_range(SCATTER_RANGE_XZ)

                if CLAMP_Y then
                    ny = clamp(ny, Y_MIN, Y_MAX)
                end

                tr:SetPosition(nx, ny, nz)
            else
                log("WARN: no Transform for chair part:", id)
            end
        end

        ----------------------------------------------------------------
        -- 2️⃣ Drop entity 522
        ----------------------------------------------------------------
        local trDrop = Query.GetTransform(DROP_ENTITY_ID)
        if trDrop then
            local x, _, z = trDrop:GetPosition()
            trDrop:SetPosition(x, -1000.0, z)
            log("Dropped entity", DROP_ENTITY_ID)
        end

        ----------------------------------------------------------------
        -- 3️⃣ Hide entity 209
        ----------------------------------------------------------------
        local trHide = Query.GetTransform(HIDE_ENTITY_ID)
        if trHide then
            trHide:SetScale(0.0, 0.0, 0.0)
            log("Hid entity", HIDE_ENTITY_ID)
        end

        ----------------------------------------------------------------
        -- 4️⃣ Scale entity 378 to 0.6
        ----------------------------------------------------------------
        local tr378 = Query.GetTransform(SCALE_378_ID)
        if tr378 then
            tr378:SetScale(0.6, 0.6, 0.6)
            log("Scaled entity", SCALE_378_ID, "to 0.6")
        end

        ----------------------------------------------------------------
        -- 5️⃣ Scale entity 380 to 1.0
        ----------------------------------------------------------------
        local tr380 = Query.GetTransform(SCALE_380_ID)
        if tr380 then
            tr380:SetScale(1.0, 1.0, 1.0)
            log("Scaled entity", SCALE_380_ID, "to 1.0")
        end
    end

    log("Ready. EntityID:", EntityID)
end

function on_update(Self, dt) end

function on_destroy(Self)
    if SHARED.ChairScatter and SHARED.ChairScatter._self == Self then
        SHARED.ChairScatter._self = nil
    end
end
