-- VolumeMasterButtons.lua
-- Expects ButtonComponent.event_trigger to be:
--   "VolumeChange1" .. "VolumeChange10"
-- Your C++ builds handlerName as: "on_button_" .. Event_Name

print("[VolumeMasterButtons] SCRIPT LOADED")

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function setMasterVolume(vol)
    vol = clamp(vol, 0.0, 1.0)
    SHARED.masterVol = vol

    -- Master only
    Audio_SetMasterVolume(vol)

    print(string.format("[VolumeMasterButtons] Master volume set to %.2f", vol))
end

function on_create(self)
    print("[VolumeMasterButtons] on_create")

    if SHARED.masterVol == nil then
        SHARED.masterVol = 1.0
    end

    print(string.format("[VolumeMasterButtons] initial masterVol=%.2f", SHARED.masterVol))

    -- Apply initial master volume once
    Audio_SetMasterVolume(SHARED.masterVol)
end

-- Helper used by all 10 handlers
local function handleVolumeChange(step)
    -- step: 1..10 -> vol 0.1..1.0
    local vol = step / 10.0
    setMasterVolume(vol)
end

-- 10 button handlers (match event_trigger names)
function on_button_VolumeChange1(self)  handleVolumeChange(1)  end
function on_button_VolumeChange2(self)  handleVolumeChange(2)  end
function on_button_VolumeChange3(self)  handleVolumeChange(3)  end
function on_button_VolumeChange4(self)  handleVolumeChange(4)  end
function on_button_VolumeChange5(self)  handleVolumeChange(5)  end
function on_button_VolumeChange6(self)  handleVolumeChange(6)  end
function on_button_VolumeChange7(self)  handleVolumeChange(7)  end
function on_button_VolumeChange8(self)  handleVolumeChange(8)  end
function on_button_VolumeChange9(self)  handleVolumeChange(9)  end
function on_button_VolumeChange10(self) handleVolumeChange(10) end

function on_destroy(self)
    print("[VolumeMasterButtons] on_destroy")
end
