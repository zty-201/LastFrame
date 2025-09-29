-- Hello.lua
local M = {}

function on_create(self)
    self.count = 0
    print("[Lua] on_create called; count=0")
end

function on_update(self, dt)
    self.count = self.count + 1
    if self.count % 60 == 0 then
        print(string.format("[Lua] on_update tick=%d (dt=%.3f)", self.count, dt))
    end
end

function on_destroy(self)
    print(string.format("[Lua] on_destroy; final count=%d", self.count or -1))
end
