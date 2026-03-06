----------------------------------------------------------------
-- ui_inventory.lua
-- Inventory UI driver — works with the engine's UIComponent system.
--
-- HOW THIS WORKS:
--   The engine renders UI by reading UIComponent.textureid on each entity.
--   Lua cannot issue arbitrary draw calls — it can only mutate UIComponent
--   fields (SetVisible, SetPosition, SetSize, SetColor) on pre-placed
--   entities whose textures were already assigned in the editor.
--
-- SETUP IN THE EDITOR (required before this script works):
--   1. Place a "panel background" entity (UIComponent with your panel
--      texture).  Note its entity ID → set ENTITY_PANEL below.
--   2. Place SLOT_COUNT "slot frame" entities in a grid (UIComponent with
--      the empty-slot-frame texture).  Note their IDs → SLOT_ENTITIES.
--   3. Place SLOT_COUNT "item icon" entities at the same grid positions
--      (UIComponent — texture is the item icon from your atlas, assigned
--      per-item in the editor).  Note their IDs → ICON_ENTITIES.
--   4. Place a "selection highlight" entity → ENTITY_SELECTED_HIGHLIGHT.
--   5. Place a "drag ghost" entity → ENTITY_DRAG_GHOST.
--   6. (Optional) Place text-label entities for item name / description
--      if your engine has a text UIComponent variant.
--
-- TEXTURE ASSIGNMENT WORKFLOW:
--   In the editor, assign the correct item icon texture to each ICON entity
--   (one icon entity = one item type).  The slotEntityId field in
--   inventory.lua's ITEM_REGISTRY records which editor entity carries
--   that item's icon.  At runtime, this script moves those icon entities
--   into the correct grid slot position so the right texture appears.
--
-- COORDINATE SYSTEM:
--   UIComponent.position is real pixels from bottom-left — matching the
--   ortho projection in GraphicsSystem::RenderUI().
--   SHARED.UI.realW / realH (from ui_canvas.lua) give the window size.
--
-- DEPENDENCIES:
--   ui_canvas.lua  → populates SHARED.UI
--   inventory.lua  → populates SHARED.Inventory
----------------------------------------------------------------

----------------------------------------------------------------
-- !! CONFIGURE THESE TO MATCH YOUR EDITOR ENTITY IDs !!
-- Open the Object Manager in the editor, select each entity you
-- placed for the inventory panel, and read off its numeric ID.
-- Replace the nil values below with those IDs.
----------------------------------------------------------------

-- Main background panel entity (dark panel texture).
local ENTITY_PANEL = nil              -- e.g. 10

-- Close / Use / Drop button entities (buttons placed on the panel).
local ENTITY_BTN_CLOSE = nil          -- e.g. 11
local ENTITY_BTN_USE   = nil          -- e.g. 12
local ENTITY_BTN_DROP  = nil          -- e.g. 13

-- Selection highlight entity (bright border, shown over selected slot).
local ENTITY_SELECTED_HIGHLIGHT = nil -- e.g. 14

-- Drag ghost entity (follows cursor while dragging an item).
local ENTITY_DRAG_GHOST = nil         -- e.g. 15

-- Optional text-label entities (only if your engine supports text UI).
local ENTITY_INFO_NAME = nil          -- e.g. 16
local ENTITY_INFO_DESC = nil          -- e.g. 17

-- Slot frame entities — one per grid cell, in row-major order.
-- These carry the empty-slot-frame texture; they are always visible
-- while the panel is open.
local SLOT_ENTITIES = {
    nil, nil, nil, nil, nil,   -- row 1  (slots  1– 5)
    nil, nil, nil, nil, nil,   -- row 2  (slots  6–10)
    nil, nil, nil, nil, nil,   -- row 3  (slots 11–15)
    nil, nil, nil, nil, nil,   -- row 4  (slots 16–20)
}

-- Item icon entities — one per grid cell, same order as SLOT_ENTITIES.
-- Each entity already has the correct item icon texture assigned in the
-- editor.  At runtime, this script repositions them into the matching
-- slot and shows/hides them based on what is in that slot.
--
-- IMPORTANT: the number of icon entities equals the number of distinct
-- item types in your game, NOT the number of grid slots.  If you have
-- fewer icon entities than slots, some slots will never display an icon.
-- Expand this table to cover every item in ITEM_REGISTRY (inventory.lua).
local ICON_ENTITIES = {
    -- Map item IDs to their editor entity IDs.
    -- Format: ["item_id"] = entityId
    -- Example:
    --   ["health_potion"] = 20,
    --   ["mana_potion"]   = 21,
    --   ["iron_sword"]    = 22,
}

----------------------------------------------------------------
-- Layout constants (pixels — must match what you placed in editor)
----------------------------------------------------------------
local SLOT_SIZE     = 90.0    -- width and height of each slot entity
local SLOT_PAD      = 12.0    -- gap between slots in the grid

----------------------------------------------------------------
-- Tint colours applied via UIComponent:SetColor(r, g, b, a)
-- The base texture colour is multiplied by this, so white = no tint.
----------------------------------------------------------------
local COL_SLOT_NORMAL   = { 1.0, 1.0, 1.0, 1.0 }
local COL_SLOT_HOVER    = { 0.8, 0.9, 1.0, 1.0 }
local COL_SLOT_SELECTED = { 0.4, 0.7, 1.0, 1.0 }
local COL_SLOT_DRAG_SRC = { 1.0, 0.8, 0.3, 0.5 }
local COL_SLOT_DROP_TGT = { 0.4, 1.0, 0.5, 1.0 }
local COL_DRAG_GHOST    = { 1.0, 1.0, 1.0, 0.65 }

----------------------------------------------------------------
-- Runtime state
----------------------------------------------------------------
local isOpen        = false
local slotOrder     = {}   -- sorted item-ID list rebuilt each frame
local selectedId    = nil  -- item ID of selected slot
local hoveredIdx    = nil  -- 1-based slot index under cursor
local dragIdx       = nil  -- 1-based slot index being dragged
local dragX, dragY  = 0, 0

local prevMouseDown = false
local prevMouseUp   = false
local prevInvKey    = false

-- Cached slot world-pixel rects { x, y, w, h } indexed by slot number.
-- Populated from editor entity positions when the panel is opened.
local slotRects = {}

----------------------------------------------------------------
-- Helpers
----------------------------------------------------------------
local function getShared()    return rawget(_G, "SHARED") end
local function getInventory() local S = getShared(); return S and S.Inventory end

-- True pixel window size (set by ui_canvas.lua every frame).
local function getScreenSize()
    local S  = getShared()
    local ui = S and S.UI
    return (ui and ui.realW) or 1920.0,
           (ui and ui.realH) or 1080.0
end

-- Raw mouse Y is pixels-from-top; engine UIComponent expects pixels-from-bottom.
local function toEngineY(rawMy)
    local _, h = getScreenSize()
    return h - rawMy
end

local function pointInRect(px, py, rx, ry, rw, rh)
    return px >= rx and px <= rx + rw and py >= ry and py <= ry + rh
end

-- Fetch UIComponent for an arbitrary entity ID using the C++ binding.
local function getUI(entityId)
    if not entityId then return nil end
    return GetEntityUI(entityId)
end

local function uiSetVisible(entityId, v)
    local ui = getUI(entityId)
    if ui then ui:SetVisible(v) end
end

local function uiSetColor(entityId, col)
    local ui = getUI(entityId)
    if ui then ui:SetColor(col[1], col[2], col[3], col[4] or 1.0) end
end

local function uiSetPosition(entityId, x, y)
    local ui = getUI(entityId)
    if ui then ui:SetPosition(x, y) end
end

local function uiGetRect(entityId)
    local ui = getUI(entityId)
    if not ui then return nil end
    local x, y = ui:GetPosition()
    local w, h = ui:GetSize()
    return { x = x, y = y, w = w, h = h }
end

----------------------------------------------------------------
-- Slot order — stable alphabetical sort of current item IDs
----------------------------------------------------------------
local function refreshSlotOrder()
    local items = {}
    local inv = getInventory()
    if inv then items = inv.GetItems() end
    local ids = {}
    for id in pairs(items) do ids[#ids + 1] = id end
    table.sort(ids)
    slotOrder = ids
end

-- Returns (itemId, itemData) for slot index i, or (nil, nil)
local function getItemAt(i)
    local inv = getInventory()
    if not inv then return nil, nil end
    local id = slotOrder[i]
    if not id then return nil, nil end
    local data = inv.GetItems()[id]
    return id, data
end

----------------------------------------------------------------
-- Cache slot pixel rects from editor entity positions.
-- Call once when opening the panel so hit-testing uses real coords.
----------------------------------------------------------------
local function cacheSlotRects()
    slotRects = {}
    for i, entityId in ipairs(SLOT_ENTITIES) do
        slotRects[i] = uiGetRect(entityId)
    end
end

local function slotIndexAt(mx, my)
    for i, rect in ipairs(slotRects) do
        if rect and pointInRect(mx, my, rect.x, rect.y, rect.w, rect.h) then
            return i
        end
    end
    return nil
end

----------------------------------------------------------------
-- Open / Close panel
----------------------------------------------------------------
local function openPanel()
    isOpen     = true
    selectedId = nil
    dragIdx    = nil

    refreshSlotOrder()
    cacheSlotRects()

    uiSetVisible(ENTITY_PANEL, true)
    uiSetVisible(ENTITY_BTN_CLOSE, true)
    uiSetVisible(ENTITY_BTN_USE,   false)  -- shown only when item selected
    uiSetVisible(ENTITY_BTN_DROP,  false)
    uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, false)
    uiSetVisible(ENTITY_DRAG_GHOST, false)
    uiSetVisible(ENTITY_INFO_NAME, false)
    uiSetVisible(ENTITY_INFO_DESC, false)

    for _, e in ipairs(SLOT_ENTITIES) do uiSetVisible(e, true)  end
    for _, e in pairs(ICON_ENTITIES)  do uiSetVisible(e, false) end

    Input.SetCursorVisible(true)
    Input.SetCursorLocked(false)
    print("[ui_inventory] Opened")
end

local function closePanel()
    isOpen     = false
    selectedId = nil
    dragIdx    = nil

    uiSetVisible(ENTITY_PANEL, false)
    uiSetVisible(ENTITY_BTN_CLOSE, false)
    uiSetVisible(ENTITY_BTN_USE,   false)
    uiSetVisible(ENTITY_BTN_DROP,  false)
    uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, false)
    uiSetVisible(ENTITY_DRAG_GHOST, false)
    uiSetVisible(ENTITY_INFO_NAME, false)
    uiSetVisible(ENTITY_INFO_DESC, false)

    for _, e in ipairs(SLOT_ENTITIES) do uiSetVisible(e, false) end
    for _, e in pairs(ICON_ENTITIES)  do uiSetVisible(e, false) end

    Input.SetCursorVisible(false)
    Input.SetCursorLocked(true)
    print("[ui_inventory] Closed")
end

----------------------------------------------------------------
-- Drive all UIComponent entities to reflect current state.
-- Called every frame while the panel is open.
----------------------------------------------------------------
local function updateVisuals(mx, my)
    local inv   = getInventory()
    local items = inv and inv.GetItems() or {}

    -- ── Slot frames and icon entities ──────────────────────────
    -- First, hide every icon entity so we start clean.
    for _, iconEntity in pairs(ICON_ENTITIES) do
        uiSetVisible(iconEntity, false)
    end

    for i = 1, #SLOT_ENTITIES do
        local slotEntity = SLOT_ENTITIES[i]
        local itemId, itemData = getItemAt(i)

        local isHovered  = (hoveredIdx == i)
        local isSelected = (selectedId ~= nil and itemId == selectedId)
        local isDragSrc  = (dragIdx == i)
        local isDragTgt  = (dragIdx ~= nil and isHovered and dragIdx ~= i)

        -- Slot frame tint
        local slotCol
        if isDragSrc       then slotCol = COL_SLOT_DRAG_SRC
        elseif isDragTgt   then slotCol = COL_SLOT_DROP_TGT
        elseif isSelected  then slotCol = COL_SLOT_SELECTED
        elseif isHovered   then slotCol = COL_SLOT_HOVER
        else                    slotCol = COL_SLOT_NORMAL
        end
        uiSetColor(slotEntity, slotCol)

        -- Icon: if this slot has an item, move its icon entity here and show it.
        if itemData and not isDragSrc then
            local iconEntity = ICON_ENTITIES[itemId]
            if iconEntity and slotRects[i] then
                uiSetPosition(iconEntity, slotRects[i].x, slotRects[i].y)
                uiSetVisible(iconEntity, true)
                uiSetColor(iconEntity, { 1, 1, 1, 1 })
            end
        end
    end

    -- ── Selection highlight ─────────────────────────────────────
    if selectedId and ENTITY_SELECTED_HIGHLIGHT then
        local selIdx = nil
        for i, id in ipairs(slotOrder) do
            if id == selectedId then selIdx = i; break end
        end
        if selIdx and slotRects[selIdx] then
            local r = slotRects[selIdx]
            uiSetPosition(ENTITY_SELECTED_HIGHLIGHT, r.x, r.y)
            uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, true)
        else
            uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, false)
        end
    else
        uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, false)
    end

    -- ── Drag ghost ──────────────────────────────────────────────
    if dragIdx and ENTITY_DRAG_GHOST then
        -- Centre the ghost on the cursor
        uiSetPosition(ENTITY_DRAG_GHOST, dragX - SLOT_SIZE * 0.5, dragY - SLOT_SIZE * 0.5)
        uiSetVisible(ENTITY_DRAG_GHOST, true)
        uiSetColor(ENTITY_DRAG_GHOST, COL_DRAG_GHOST)
    else
        uiSetVisible(ENTITY_DRAG_GHOST, false)
    end

    -- ── Action buttons (USE / DROP) ─────────────────────────────
    local hasSelection = (selectedId ~= nil and items[selectedId] ~= nil)
    uiSetVisible(ENTITY_BTN_USE,  hasSelection)
    uiSetVisible(ENTITY_BTN_DROP, hasSelection)

    -- ── Info panel ─────────────────────────────────────────────
    -- If your engine exposes SetText() on text entities, call it here:
    --   getUI(ENTITY_INFO_NAME):SetText(items[selectedId].name)
    --   getUI(ENTITY_INFO_DESC):SetText(items[selectedId].description)
    uiSetVisible(ENTITY_INFO_NAME, hasSelection)
    uiSetVisible(ENTITY_INFO_DESC, hasSelection)
    if hasSelection then
        local d = items[selectedId]
        -- Log as fallback if no text component available
        -- (only print on change — use a dirty flag in production)
    end
end

----------------------------------------------------------------
-- on_create
----------------------------------------------------------------
function on_create(Self)
    print("[ui_inventory] on_create(), entity:", EntityID)

    -- Start fully hidden
    uiSetVisible(ENTITY_PANEL, false)
    uiSetVisible(ENTITY_BTN_CLOSE, false)
    uiSetVisible(ENTITY_BTN_USE,   false)
    uiSetVisible(ENTITY_BTN_DROP,  false)
    uiSetVisible(ENTITY_SELECTED_HIGHLIGHT, false)
    uiSetVisible(ENTITY_DRAG_GHOST, false)
    uiSetVisible(ENTITY_INFO_NAME, false)
    uiSetVisible(ENTITY_INFO_DESC, false)
    for _, e in ipairs(SLOT_ENTITIES) do uiSetVisible(e, false) end
    for _, e in pairs(ICON_ENTITIES)  do uiSetVisible(e, false) end

    print("[ui_inventory] Ready — fill in entity IDs at top of file.")
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)

    -- ── Toggle inventory key ──────────────────────────────────
    local invKeyDown = Key_Actions
        and Input.GetActionDown(Key_Actions.INVENTORY)
        or  Input.GetKeyDown(73)   -- fallback: I key

    if invKeyDown and not prevInvKey then
        if isOpen then closePanel() else openPanel() end
    end
    prevInvKey = invKeyDown

    if not isOpen then return end

    -- ── Mouse coords in engine pixel space (bottom-left origin) ─
    local rawMx, rawMy = Input.GetMousePosition()
    local mx = rawMx
    local my = toEngineY(rawMy)

    -- ── Refresh data ──────────────────────────────────────────
    refreshSlotOrder()
    hoveredIdx = slotIndexAt(mx, my)

    -- ── Input processing ──────────────────────────────────────
    local mouseDown = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local mouseHeld = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local mouseUp   = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)

    -- Track drag ghost position
    if dragIdx and mouseHeld then
        dragX = mx
        dragY = my
    end

    -- Mouse DOWN
    if mouseDown and not prevMouseDown then

        -- Close button
        local closeBtnRect = uiGetRect(ENTITY_BTN_CLOSE)
        if closeBtnRect and pointInRect(mx, my, closeBtnRect.x, closeBtnRect.y, closeBtnRect.w, closeBtnRect.h) then
            closePanel()
            prevMouseDown = mouseDown
            return
        end

        -- USE button
        local useBtnRect = uiGetRect(ENTITY_BTN_USE)
        if selectedId and useBtnRect and pointInRect(mx, my, useBtnRect.x, useBtnRect.y, useBtnRect.w, useBtnRect.h) then
            print("[ui_inventory] USE:", selectedId)
            local S = getShared()
            if S and S.Game and S.Game.UseItem then S.Game.UseItem(selectedId) end
            prevMouseDown = mouseDown
            return
        end

        -- DROP button
        local dropBtnRect = uiGetRect(ENTITY_BTN_DROP)
        if selectedId and dropBtnRect and pointInRect(mx, my, dropBtnRect.x, dropBtnRect.y, dropBtnRect.w, dropBtnRect.h) then
            print("[ui_inventory] DROP:", selectedId)
            local inv = getInventory()
            if inv then
                inv.Remove(selectedId, 1)
                if inv.Count(selectedId) <= 0 then selectedId = nil end
            end
            prevMouseDown = mouseDown
            return
        end

        -- Slot click / begin drag
        if hoveredIdx then
            local itemId, _ = getItemAt(hoveredIdx)
            if itemId then
                dragIdx    = hoveredIdx
                dragX, dragY = mx, my
                selectedId = itemId
            else
                selectedId = nil
                dragIdx    = nil
            end
        else
            selectedId = nil
            dragIdx    = nil
        end
    end

    -- Mouse UP — resolve drag drop / combine
    if mouseUp and not prevMouseUp then
        if dragIdx then
            local dropIdx = slotIndexAt(mx, my)

            if dropIdx and dropIdx ~= dragIdx then
                local srcId = getItemAt(dragIdx)
                local tgtId = getItemAt(dropIdx)

                if srcId and tgtId then
                    local inv = getInventory()
                    if inv then
                        local recipe = inv.GetRecipe(srcId, tgtId)
                        if recipe then
                            local ok = inv.Combine(srcId, tgtId)
                            if ok then
                                print("[ui_inventory] Combined =>", recipe.name)
                                selectedId = recipe.id
                            end
                        else
                            print("[ui_inventory] No recipe:", srcId, "+", tgtId)
                        end
                    end
                end

            elseif dropIdx == dragIdx then
                -- Dropped back on same slot — keep selection
                local itemId = getItemAt(dragIdx)
                selectedId   = itemId or selectedId
            end

            dragIdx = nil
        end
    end

    prevMouseDown = mouseDown
    prevMouseUp   = mouseUp

    -- ── Drive all UI entities ─────────────────────────────────
    updateVisuals(mx, my)
end

----------------------------------------------------------------
-- on_destroy
----------------------------------------------------------------
function on_destroy(Self)
    if isOpen then
        Input.SetCursorVisible(false)
        Input.SetCursorLocked(true)
    end
    print("[ui_inventory] on_destroy()")
end
