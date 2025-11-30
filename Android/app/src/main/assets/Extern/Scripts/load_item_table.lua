-- =========================================================
-- load_items.lua
-- Loads ITEMS table ONE TIME globally.
-- Attach this to a GameManager object in your scene.
-- =========================================================

function on_create(self)
    print("[load_items] Initializing item database...")

    -- Use dofile or require depending on engine rules
    dofile("items.lua")      -- forces 1-time execution from file

    if ITEMS then
        print("[load_items] ITEMS loaded. Total entries:", #ITEMS)
    else
        print("[load_items] ERROR: ITEMS table missing!")
    end
end

function on_update(self, dt)
    -- does nothing; only loads once on_create
end
