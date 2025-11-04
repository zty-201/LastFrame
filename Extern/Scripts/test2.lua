-- ======================================================
-- test2.lua
-- A secondary script to verify multi-script loading
-- ======================================================

function on_create()
    print("[test2.lua] on_create called")
end

function on_update(dt)
    if dt then
        print(string.format("[test2.lua] on_update called, dt = %.3f", dt))
    end
end

function on_destroy()
    print("[test2.lua] on_destroy called")
end

-- Additional exported functions for ScriptDebug test
function greet2(name)
    return "[test2.lua] Greetings, " .. name .. "!"
end

function add2(a, b)
    print(string.format("[test2.lua] Adding %d + %d", a, b))
    return a + b
end
