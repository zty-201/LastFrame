----------------------------------------------------------------
-- LockButton.lua
-- Attach to any NumberX_Pressed OR NumberX_Unpressed entity.
--
-- Setup per entity:
--   1. Set PARTNER_ID to the paired entity's ID (the other state).
--   2. Add a ButtonComponent with event_trigger = "NumberButtonToggle"
--
-- When clicked: hides self, shows partner.
--
-- Entity ID pairs (from LockScreen.json):
--   Number0:  Unpressed=824  <-> Pressed=823
--   Number1:  Unpressed=819  <-> Pressed=820
--   Number2:  Unpressed=822  <-> Pressed=821
--   Number3:  Unpressed=826  <-> Pressed=825
--   Number4:  Unpressed=828  <-> Pressed=827
--   Number5:  Unpressed=830  <-> Pressed=829
--   Number6:  Unpressed=832  <-> Pressed=831
--   Number7:  Unpressed=834  <-> Pressed=833
--   Number8:  Unpressed=836  <-> Pressed=835
--   Number9:  Unpressed=838  <-> Pressed=837
----------------------------------------------------------------

local PARTNER_ID = 824   -- <<< CHANGE THIS per entity attachment

-- Debounce: ignore re-fires within this window (seconds).
-- Prevents mobile multi-touch from double-toggling a button back to unpressed.
local DEBOUNCE_TIME = 0.3
local debounceTimer = 0.0

function on_create(self)
    debounceTimer = 0.0
end

function on_update(self, dt)
    if debounceTimer > 0.0 then
        debounceTimer = debounceTimer - (dt or 0.016)
    end
end

function on_button_NumberButtonToggle(self)
    if debounceTimer > 0.0 then return end
    debounceTimer = DEBOUNCE_TIME

    -- Hide self (set size_NDC to 0,0)
    if EntityUI then
        EntityUI:SetSizeNDC(0.0, 0.0)
    end

    -- Show partner (restore size_NDC to 0.1,0.1)
    local partner = GetEntityUI(PARTNER_ID)
    if partner then
        partner:SetSizeNDC(0.1, 0.1)
    end
end
