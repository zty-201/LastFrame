-- Attach this to the Player entity.
-- Requires your input mapping name "Interact" (like E).

function on_create(self)
  SHARED.interact_request = nil
  SHARED.player_id = SHARED.player_id or EntityID
end

local function getShared()
    return rawget(_G, "SHARED")
end
local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.UI = Shared.UI or {}
    return Shared.UI
end

function on_update(self, dt)
  -- Press E (mapped action name) to interact

  local uiShared = ensureUIShared()
  if not uiShared then return end

  local interact = uiShared.actionRequested

  if interact or Input.GetActionDown("INTERACT") then
    --print("=====  hello player interact 2.0 =====\n")
    -- interact = false
    -- uiShared.actionRequested = interact;

    local targetId = SHARED.zone_target or 0
    print("=====  hello player interact 2.0 =====", targetId)
    if targetId ~= 0 then
      -- Broadcast a request. Interactable objects will consume it.
      SHARED.interact_request = {
        target = targetId,
        player = EntityID
      }
      --print("Interact request -> target:", targetId)
    end
  end
end
