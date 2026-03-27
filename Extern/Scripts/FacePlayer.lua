----------------------------------------------------------------
-- FacePlayer.lua
-- Attach to entity 834
-- Rotates the entity on the Y axis every frame to face the player
----------------------------------------------------------------

local PLAYER_ENTITY_ID = 5

local frameCount = 0

function on_create(self)
    print("[FacePlayer] ===== LOADED on entity=" .. tostring(EntityID) .. " =====")
end

function on_update(self, dt)
    frameCount = frameCount + 1

    local playerTr = Query.GetTransform(PLAYER_ENTITY_ID)
    if not playerTr then
        if frameCount % 60 == 0 then
            print("[FacePlayer] ERROR: Query.GetTransform(" .. PLAYER_ENTITY_ID .. ") returned nil")
        end
        return
    end

    local myTr = Query.GetTransform(EntityID)
    if not myTr then
        if frameCount % 60 == 0 then
            print("[FacePlayer] ERROR: Query.GetTransform(" .. tostring(EntityID) .. ") returned nil")
        end
        return
    end

    local px, py, pz = playerTr:GetPosition()
    local ex, ey, ez = myTr:GetPosition()

    local dx = px - ex
    local dz = pz - ez

    if frameCount % 60 == 0 then
        print(string.format("[FacePlayer] player=(%.1f, %.1f, %.1f)  entity=(%.1f, %.1f, %.1f)  dx=%.2f dz=%.2f",
            px, py, pz, ex, ey, ez, dx, dz))
    end

    if math.abs(dx) < 0.001 and math.abs(dz) < 0.001 then return end

    local yaw = math.deg(math.atan(dx, dz))

    if frameCount % 60 == 0 then
        print("[FacePlayer] yaw=" .. string.format("%.2f", yaw))
    end

    if Transform_SetRotation then
        Transform_SetRotation(EntityID, 0.0, yaw, 0.0)
    else
        print("[FacePlayer] ERROR: Transform_SetRotation is nil!")
    end
end
