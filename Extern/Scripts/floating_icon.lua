local ICON_ENTITY_ID = 200        -- UI element
local TARGET_ENTITY_ID = 29       -- object to float above
local PLAYER_ID = 5
local SHOW_RANGE = 100

function on_update(self, dt)
    local ui = EntityUI
    if not ui then return end

    local player = GetEntityTransform(PLAYER_ID)
    local target = GetEntityTransform(TARGET_ENTITY_ID)
    if not player or not target then return end

    local px,py,pz = player:GetPosition()
    local tx,ty,tz = target:GetPosition()

    local dx = px - tx
    local dy = py - ty
    local dz = pz - tz
    local dist = math.sqrt(dx*dx + dy*dy + dz*dz)

    if dist < SHOW_RANGE then
        ui:SetVisible(true)

        -- crude fake floating UI (top center of the screen)
        ui:SetPosition(350, 50)

    else
        ui:SetVisible(false)
    end
end
