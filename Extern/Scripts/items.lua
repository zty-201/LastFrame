ITEMS = {
    -- =========================================================
    -- KEYS
    -- =========================================================
    [1001] = {
        id = 1001,
        name = "Rusty Key",
        type = "key",
        maxStack = 1,
        description = "An old rusty key. It opens a certain door.",
        icon = "icons/key_rusty.png",
        canPickup = true,
    },

    [1002] = {
        id = 1002,
        name = "Music Box Key",
        type = "key",
        maxStack = 1,
        description = "A tiny decorative key used to wind a music box.",
        icon = "icons/key_musicbox.png",
        canPickup = true,
    },

    -- =========================================================
    -- NON-INVENTORY INTERACTABLES
    -- (Cannot be picked up. Only examined or used)
    -- =========================================================
    [2001] = {
        id = 2001,
        name = "Teddy With Note",
        type = "interactable",
        maxStack = 1,
        description = "A teddy bear holding a hidden note.",
        icon = "icons/teddy_note.png",
        canPickup = false,  -- IMPORTANT: cannot go into inventory
    },

    -- =========================================================
    -- CRAFTING MATERIALS
    -- =========================================================
    [3001] = {
        id = 3001,
        name = "Metal Pick",
        type = "material",
        maxStack = 10,
        description = "A small metal pick. Used to craft a lockpick.",
        icon = "icons/metal_pick.png",
        canPickup = true,
        craftable = true,
    },

    [3002] = {
        id = 3002,
        name = "Allen Key",
        type = "material",
        maxStack = 10,
        description = "A small hex key. Combine with a metal pick.",
        icon = "icons/allen_key.png",
        canPickup = true,
        craftable = true,
    },

    -- =========================================================
    -- FINAL CRAFTED TOOL
    -- =========================================================
    [4001] = {
        id = 4001,
        name = "Lockpick",
        type = "tool",
        maxStack = 1,
        description = "A basic lockpick crafted from two components.",
        icon = "icons/lockpick.png",
        canPickup = true,
        craftable = false,
        recipe = {
            { id = 3001, qty = 1 }, -- Metal Pick
            { id = 3002, qty = 1 }, -- Allen Key
        }
    }
}
