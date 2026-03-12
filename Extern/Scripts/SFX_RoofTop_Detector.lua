print("[SFX_Rooftop_Detector] script loaded")

local DEBUG = true
local PLAYER_ID = 5

function on_create(self)
    _G.ROOFTOP_COUNT = _G.ROOFTOP_COUNT or 0

    if DEBUG then
        print("[SFX_Rooftop_Detector] Ready EntityID =", EntityID)
    end
end

function on_trigger_enter(self, otherId)
    if otherId ~= PLAYER_ID then return end

    _G.ROOFTOP_COUNT = (_G.ROOFTOP_COUNT or 0) + 1

    if DEBUG then
        print("[SFX_Rooftop_Detector] ENTER -> ROOFTOP_COUNT =", _G.ROOFTOP_COUNT)
    end
end

function on_trigger_exit(self, otherId)
    if otherId ~= PLAYER_ID then return end

    _G.ROOFTOP_COUNT = (_G.ROOFTOP_COUNT or 0) - 1
    if _G.ROOFTOP_COUNT < 0 then _G.ROOFTOP_COUNT = 0 end

    if DEBUG then
        print("[SFX_Rooftop_Detector] EXIT -> ROOFTOP_COUNT =", _G.ROOFTOP_COUNT)
    end
end