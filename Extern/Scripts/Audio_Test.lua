-- Audio_Test.lua

local v = 1.0

function on_create(Self)
    print("[Audio_Test] EntityID:", EntityID)
    print("[Audio_Test] Press Z/X/C to play clip 0/1/2")
    print("[Audio_Test] Press - / = to change volume")
end

function on_update(Self, dt)
    -- Play clips
    if Input.GetKeyDown(GLFW_KEY_Z) then
        Audio.Play(EntityID, 0)
        print("[Audio_Test] Play clip 0")
    end

    if Input.GetKeyDown(GLFW_KEY_X) then
        Audio.Play(EntityID, 1)
        print("[Audio_Test] Play clip 1")
    end

    if Input.GetKeyDown(GLFW_KEY_C) then
        Audio.Play(EntityID, 2)
        print("[Audio_Test] Play clip 2")
    end

    -- Volume control (clamp 0..1)
    if Input.GetKeyDown(GLFW_KEY_MINUS) then
        v = v - 0.1
        if v < 0 then v = 0 end
        Audio.SetVolume(EntityID, 0, v)
        Audio.SetVolume(EntityID, 1, v)
        Audio.SetVolume(EntityID, 2, v)
        print(string.format("[Audio_Test] Volume -> %.2f (set on clips 0/1/2)", v))
    end

    if Input.GetKeyDown(GLFW_KEY_EQUAL) then
        v = v + 0.1
        if v > 1 then v = 1 end
        Audio.SetVolume(EntityID, 0, v)
        Audio.SetVolume(EntityID, 1, v)
        Audio.SetVolume(EntityID, 2, v)
        print(string.format("[Audio_Test] Volume -> %.2f (set on clips 0/1/2)", v))
    end
end
