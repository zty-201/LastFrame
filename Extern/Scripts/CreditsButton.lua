-- CreditsButton.lua
-- Triggers the StateMachine transition to the Credits state when Credits button is clicked

print("[CreditsButton] SCRIPT LOADED")

local function ui_nav_locked()
    return (_G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0)
end

function on_create(self)
    print("[CreditsButton] on_create | EntityID=" .. tostring(EntityID))

    if StateMachine_SetTrigger == nil then
        print("[CreditsButton] ERROR: StateMachine_SetTrigger is NIL (binding missing)")
    else
        print("[CreditsButton] StateMachine_SetTrigger exists")
    end
end

function on_update(self, dt)
    if _G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0 then
        dt = dt or 0.016
        _G.__UI_NAV_LOCK = _G.__UI_NAV_LOCK - dt
        if _G.__UI_NAV_LOCK <= 0.0 then
            _G.__UI_NAV_LOCK = 0.0
        end
    end
end

function on_button_Credits(self)
    if ui_nav_locked() then
        print("[CreditsButton] Ignored click (UI nav lock active)")
        return
    end

    print("[CreditsButton] Credits button clicked -> triggering ToEndCredit")

    -- Lock navigation briefly to prevent double-firing
    _G.__UI_NAV_LOCK = 0.25

    if StateMachine_SetTrigger == nil then
        print("[CreditsButton] ERROR: StateMachine_SetTrigger binding missing")
        return
    end

    local ok, err = pcall(function()
        StateMachine_SetTrigger("ToEndCredit")
    end)

    if ok then
        print("[CreditsButton] Requested MainMenu -> Credits")
    else
        print("[CreditsButton] ERROR triggering StartCredits: " .. tostring(err))
    end
end

function on_destroy(self)
    print("[CreditsButton] on_destroy | EntityID=" .. tostring(EntityID))
end