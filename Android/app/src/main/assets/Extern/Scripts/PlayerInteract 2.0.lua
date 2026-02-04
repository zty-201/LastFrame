-- Attach this to the Player entity.
-- Requires your input mapping name "Interact" (like E).

function on_create(self)
  SHARED.interact_request = nil
  SHARED.player_id = SHARED.player_id or EntityID
end

function on_update(self, dt)
  -- Press E (mapped action name) to interact
  if Input.GetActionDown("Interact") then
    local targetId = SHARED.zone_target or 0
    if targetId ~= 0 then
      -- Broadcast a request. Interactable objects will consume it.
      SHARED.interact_request = {
        target = targetId,
        player = EntityID
      }
      -- print("Interact request -> target:", targetId)
    end
  end
end
