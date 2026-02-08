print("[CanteenBear_CloseButton] ===== SCRIPT FILE LOADED =====")

local BEAR_ID = 376
local UI_TABLE = "CANTEEN_BEAR_UI_OPEN"

function on_button_CloseCanteenBearUI(self)
    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[UI_TABLE][BEAR_ID] = false
    print("[CanteenBear_CloseButton] Closed UI via X button")
end
