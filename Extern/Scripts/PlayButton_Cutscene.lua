function on_create(self)
    print("[ButtonTest] on_create OK")
end

function on_play_hover(self, param)
    print("[ButtonTest] HOVER received!")
end

function on_play_pressed(self, param)
    print("[ButtonTest] CLICK received!")
end
