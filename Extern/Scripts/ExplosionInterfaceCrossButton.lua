----------------------------------------------------------------
-- ExplosionToy_CloseButton.lua
-- Attach to CrossButton UI entity (with ButtonComponent)
--
-- ButtonComponent.event_trigger must be: CloseExplosionToyUI
-- so the handler must be: on_button_CloseExplosionToyUI
----------------------------------------------------------------

print("[ExplosionToy_CloseButton] ===== SCRIPT FILE LOADED =====")

local TOY_ID = 378
local UI_TABLE = "EXPLOSION_TOY_UI_OPEN"

function on_button_CloseExplosionToyUI(self)
    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    SHARED[UI_TABLE][TOY_ID] = false
    print("[ExplosionToy_CloseButton] Closed UI via X button")
end
