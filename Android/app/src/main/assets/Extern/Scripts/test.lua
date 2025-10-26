-- test.lua - Simple test script
print("=== Lua Test Script Loaded ===")
print("Lua version: " .. _VERSION)

-- Simple functions that can be called from C++
function greet(name)
    print("Hello from Lua, " .. name .. "!")
    return "Greeting sent to " .. name
end

function add(a, b)
    local result = a + b
    print("Lua: " .. a .. " + " .. b .. " = " .. result)
    return result
end

-- Component lifecycle callbacks (if your LuaScriptComponent supports them)
function OnCreate()
    print("Lua script OnCreate() called")
end

function OnUpdate(dt)
    print("Lua OnUpdate() - deltaTime: " .. dt)
end

function OnDestroy()
    print("Lua script OnDestroy() called")
end

print("=== Test script ready ===")