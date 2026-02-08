----------------------------------------------------------------
-- BookInterface_CloseButton.lua
-- Attach to CrossButton UI entity (with ButtonComponent)
--
-- ButtonComponent.event_trigger must be: CloseBookUI
-- so the handler must be: on_button_CloseBookUI
----------------------------------------------------------------

print("[BookInterface_CloseButton] ===== SCRIPT FILE LOADED =====")

local BOOK_ID  = 431          -- <<<<<< CHANGE THIS
local UI_TABLE = "BOOK_UI_OPEN"

function on_button_CloseBookUI(self)
    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    SHARED[UI_TABLE][BOOK_ID] = false
    print("[BookInterface_CloseButton] Closed UI via X button")
end
