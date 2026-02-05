-- Interactable_Register.lua
-- Attach to an interactable TRIGGER entity (the trigger-only entity)

function on_create(self)
    SHARED.INTERACTABLE = SHARED.INTERACTABLE or {}
    SHARED.INTERACTABLE[EntityID] = true
    print("[Interactable] registered -> " .. tostring(EntityID))
end

function on_destroy(self)
    if SHARED.INTERACTABLE then
        SHARED.INTERACTABLE[EntityID] = nil
    end
    print("[Interactable] unregistered -> " .. tostring(EntityID))
end
