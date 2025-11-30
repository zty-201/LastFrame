----------------------------------------------------------------
-- audio_autoplay_loop.lua
----------------------------------------------------------------

function on_create(self)
    if not EntityAudio then
        print("[AudioAutoPlay] ERROR: No AudioComponent on this entity!")
        return
    end

    print("[AudioAutoPlay] Starting loop audio:", EntityID)

    EntityAudio:SetLoop(true)
    EntityAudio:SetVolume(1.0)
    EntityAudio:SetIs3D(true)

    -- THIS requests ECS to play it
    EntityAudio:RequestPlay()
end
