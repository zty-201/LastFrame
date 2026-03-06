----------------------------------------------------------------
-- ui_inventory.lua
-- Inventory UI Panel
--
-- HOW TEXTURES WORK:
--   Each item stored via SHARED.Inventory.Add() carries a textureName
--   field (the asset name string, e.g. "icon_sword").
--
--   This script keeps a pool of slot UIComponent entities.
--   When a slot is populated it calls:
--       slotUI:SetTexture(item.textureName)
--   which maps to the new UIComponent binding added in Script.cpp:
--       "SetTexture", [](UIComponent& ui, const std::string& name) {
--           ui.textureid = AssetManagerSystem::GetFullGuid(
--               AssetManagerSystem::TEXTURE_GUID, name); }
--
--   Empty slots call slotUI:SetTexture("") to clear the texture
--   (the renderer falls back to m_defaultWhiteTexture automatically).
--
-- Depends on:
--   ui_canvas.lua  -> SHARED.UI  (screen metrics)
--   inventory.lua  -> SHARED.Inventory  (item data)
--
-- Required C++ addition in Script.cpp (UIComponent bindings):
--   "SetTexture", [](UIComponent& ui, const std::string& name) {
--       ui.textureid = AssetManagerSystem::GetFullGuid(
--           AssetManagerSystem::TEXTURE_GUID, name);
--   }
----------------------------------------------------------------

----------------------------------------------------------------
-- Layout constants (virtual UI units, 1440-tall coordinate space)
----------------------------------------------------------------
local PANEL_W        = 600.0
local PANEL_H        = 520.0
local PANEL_ANCHOR_X = 0.5
local PANEL_ANCHOR_Y = 0.5

local SLOT_SIZE      = 90.0
local SLOT_PAD       = 12.0
local SLOTS_PER_ROW  = 5
local HEADER_H       = 50.0
local INFO_H         = 120.0

local TITLE_FONT_SZ  = 28.0
local ITEM_FONT_SZ   = 16.0
local INFO_FONT_SZ   = 18.0

local COL_PANEL_BG      = { 0.08, 0.08, 0.10, 0.95 }
local COL_HEADER_BG     = { 0.12, 0.12, 0.16, 1.00 }
local COL_SLOT_EMPTY    = { 0.15, 0.15, 0.18, 1.00 }
local COL_SLOT_FILLED   = { 0.20, 0.22, 0.28, 1.00 }
local COL_SLOT_SELECTED = { 0.35, 0.55, 0.80, 1.00 }
local COL_SLOT_DRAG_SRC = { 0.80, 0.60, 0.20, 1.00 }
local COL_SLOT_DROP_TGT = { 0.40, 0.80, 0.40, 1.00 }
local COL_SLOT_HOVER    = { 0.28, 0.30, 0.38, 1.00 }
local COL_INFO_BG       = { 0.10, 0.10, 0.13, 1.00 }
local COL_TEXT_TITLE    = { 0.90, 0.90, 1.00, 1.00 }
local COL_TEXT_ITEM     = { 0.80, 0.85, 0.95, 1.00 }
local COL_TEXT_QTY      = { 0.55, 0.85, 0.55, 1.00 }
local COL_TEXT_DESC     = { 0.70, 0.72, 0.80, 1.00 }
local COL_BTN_NORMAL    = { 0.22, 0.28, 0.40, 1.00 }
local COL_BTN_HOVER     = { 0.32, 0.42, 0.62, 1.00 }
local COL_BTN_TEXT      = { 0.90, 0.92, 1.00, 1.00 }
local COL_CLOSE_BTN     = { 0.55, 0.18, 0.18, 1.00 }
local COL_CLOSE_HOVER   = { 0.80, 0.25, 0.25, 1.00 }
local COL_COMBINE_HINT  = { 0.90, 0.75, 0.30, 1.00 }

----------------------------------------------------------------
-- Script state
----------------------------------------------------------------
local UI        = nil     -- this entity's UIComponent (panel background)
local isOpen    = false

local slotOrder = {}      -- sorted item IDs for stable slot layout

-- Interaction
local selectedId  = nil
local hoveredIdx  = nil
local dragIdx     = nil
local dragX       = 0.0
local dragY       = 0.0

local prevMouseDown = false
local prevMouseUp   = false
local prevInvKey    = false

local btnClose = nil
local btnUse   = nil
local btnDrop  = nil

local combineFlashTimer = 0.0
local combineFlashMsg   = ""

-- Pool of slot UIComponent entities created at runtime.
-- slotEntities[i] is the entt entity (uint32) for slot i.
-- We create these once in on_create and reuse them every frame.
local slotEntities = {}
local MAX_SLOTS    = 20  -- 5 cols x 4 rows

----------------------------------------------------------------
-- Shared accessors
----------------------------------------------------------------
local function getShared()  return rawget(_G, "SHARED") end
local function getUIShared() local S = getShared(); return S and S.UI end
local function getInventory() local S = getShared(); return S and S.Inventory end

local function getCanvasSize()
    local ui = getUIShared()
    return (ui and ui.screenW) or 2560.0, (ui and ui.screenH) or 1440.0
end

local function toUICoords(rawMx, rawMy)
    local ui = getUIShared()
    if ui and ui.ToUIX and ui.ToUIY then
        return ui.ToUIX(rawMx), ui.ToUIY(rawMy)
    end
    local _, h = getCanvasSize()
    return rawMx, h - rawMy
end

local function pointInRect(px, py, rx, ry, rw, rh)
    return px >= rx and px <= rx + rw and py >= ry and py <= ry + rh
end

local function getPanelOrigin()
    local sw, sh = getCanvasSize()
    return PANEL_ANCHOR_X * sw - PANEL_W * 0.5,
           PANEL_ANCHOR_Y * sh - PANEL_H * 0.5
end

-- Rebuild slotOrder from current inventory (sorted for stability)
local function refreshSlotOrder()
    local inv = getInventory()
    local items = inv and inv.GetItems() or {}
    local ids = {}
    for id in pairs(items) do ids[#ids + 1] = id end
    table.sort(ids)
    slotOrder = ids
end

local function getItemAt(idx)
    local inv = getInventory()
    if not inv then return nil, nil end
    local id = slotOrder[idx]
    if not id then return nil, nil end
    local items = inv.GetItems()
    return id, items[id]
end

-- Slot index -> bottom-left x, y in virtual-UI space
local function slotLocalPos(idx, panelX, panelY)
    local col = ((idx - 1) % SLOTS_PER_ROW)
    local row = math.floor((idx - 1) / SLOTS_PER_ROW)
    local x = panelX + SLOT_PAD + col * (SLOT_SIZE + SLOT_PAD)
    local y = panelY + INFO_H + SLOT_PAD + row * (SLOT_SIZE + SLOT_PAD)
    return x, y
end

----------------------------------------------------------------
-- Slot UIComponent entity helpers
----------------------------------------------------------------

-- Apply a texture to a slot's UIComponent.
-- Calls the new SetTexture binding added in Script.cpp.
-- If textureName is "" the renderer automatically falls back to
-- the default white texture (harmless transparent-looking quad).
local function applySlotTexture(slotEntityId, textureName)
    if not slotEntityId then return end
    local slotUI = GetEntityUI(slotEntityId)
    if not slotUI then return end

    -- SetTexture resolves the name to a full_guid via AssetManagerSystem
    -- Requires UIComponent binding in Script.cpp:
    --   "SetTexture", [](UIComponent& ui, const std::string& name) {
    --       ui.textureid = AssetManagerSystem::GetFullGuid(
    --           AssetManagerSystem::TEXTURE_GUID, name); }
    slotUI:SetTexture(textureName or "")
end

-- Position + resize a slot UIComponent to match current layout.
-- Slot entities are rendered by the engine's RenderUI() pass using
-- UIComponent.position and UIComponent.size in real-pixel space.
-- We convert from virtual-UI coords back to real pixels here.
local function positionSlotEntity(slotEntityId, sx, sy)
    if not slotEntityId then return end
    local slotUI = GetEntityUI(slotEntityId)
    if not slotUI then return end

    local uiS = getUIShared()
    -- Convert virtual-UI position to real pixels
    local realX, realY, realW, realH
    if uiS and uiS.scaleX and uiS.scaleY then
        realX = sx / uiS.scaleX
        realY = sy / uiS.scaleY
        realW = SLOT_SIZE / uiS.scaleX
        realH = SLOT_SIZE / uiS.scaleY
    else
        realX, realY, realW, realH = sx, sy, SLOT_SIZE, SLOT_SIZE
    end

    slotUI:SetPosition(realX, realY)
    slotUI:SetSize(realW, realH)
end

----------------------------------------------------------------
-- Drawing helpers (panel overlay uses EntityUI draw calls)
----------------------------------------------------------------
local function drawRect(x, y, w, h, col)
    if not UI then return end
    UI:SetColor(col[1], col[2], col[3], col[4] or 1.0)
    UI:DrawRect(x, y, w, h)
end

local function drawText(text, x, y, fontSize, col)
    if not UI then return end
    UI:SetColor(col[1], col[2], col[3], col[4] or 1.0)
    UI:DrawText(text, x, y, fontSize)
end

local function truncate(s, maxLen)
    if #s <= maxLen then return s end
    return s:sub(1, maxLen - 2) .. ".."
end

local function makeButton(label, x, y, w, h, col, hoverCol, mx, my)
    local isHover = pointInRect(mx, my, x, y, w, h)
    drawRect(x, y, w, h, isHover and hoverCol or col)
    local tw = w * 0.5 - #label * (ITEM_FONT_SZ * 0.3)
    drawText(label, x + tw, y + h * 0.28, ITEM_FONT_SZ, COL_BTN_TEXT)
    return { x = x, y = y, w = w, h = h }
end

----------------------------------------------------------------
-- Slot hit-test: return slot index (1-based) or nil
----------------------------------------------------------------
local function slotIndexAt(mx, my)
    local panelX, panelY = getPanelOrigin()
    for i = 1, MAX_SLOTS do
        local sx, sy = slotLocalPos(i, panelX, panelY)
        if sy + SLOT_SIZE > panelY + PANEL_H - HEADER_H then break end
        if pointInRect(mx, my, sx, sy, SLOT_SIZE, SLOT_SIZE) then
            return i
        end
    end
    return nil
end

----------------------------------------------------------------
-- Update slot entities: texture + position every frame
-- This is the core of the texture-loading integration.
----------------------------------------------------------------
local function updateSlotEntities()
    local panelX, panelY = getPanelOrigin()

    for i = 1, MAX_SLOTS do
        local eid = slotEntities[i]
        if not eid then break end

        local sx, sy = slotLocalPos(i, panelX, panelY)
        local slotUI = GetEntityUI(eid)
        if slotUI then
            local itemId, itemData = getItemAt(i)
            local slotVisible = isOpen and (sy + SLOT_SIZE <= panelY + PANEL_H - HEADER_H)

            slotUI:SetVisible(slotVisible)

            if slotVisible then
                -- Position in real-pixel space
                positionSlotEntity(eid, sx, sy)

                -- Apply texture from inventory item
                if itemData and itemData.textureName and itemData.textureName ~= "" then
                    applySlotTexture(eid, itemData.textureName)
                    -- Tint: white for normal, coloured for selected/drag states
                    local col = COL_SLOT_FILLED
                    if dragIdx == i      then col = COL_SLOT_DRAG_SRC
                    elseif hoveredIdx == i and dragIdx and dragIdx ~= i then col = COL_SLOT_DROP_TGT
                    elseif itemId == selectedId then col = COL_SLOT_SELECTED
                    elseif hoveredIdx == i      then col = COL_SLOT_HOVER
                    end
                    slotUI:SetColor(col[1], col[2], col[3], col[4] or 1.0)
                else
                    -- Empty slot: clear texture, use dim colour
                    applySlotTexture(eid, "")
                    slotUI:SetColor(
                        COL_SLOT_EMPTY[1], COL_SLOT_EMPTY[2],
                        COL_SLOT_EMPTY[3], COL_SLOT_EMPTY[4] or 1.0)
                end
            end
        end
    end
end

----------------------------------------------------------------
-- Render panel overlay (header, info bar, text labels)
----------------------------------------------------------------
local function render(mx, my)
    if not isOpen then return end

    local panelX, panelY = getPanelOrigin()
    local inv   = getInventory()
    local items = (inv and inv.GetItems()) or {}

    -- Panel background
    drawRect(panelX, panelY, PANEL_W, PANEL_H, COL_PANEL_BG)

    -- Header
    local headerY = panelY + PANEL_H - HEADER_H
    drawRect(panelX, headerY, PANEL_W, HEADER_H, COL_HEADER_BG)
    drawText("INVENTORY", panelX + 16, headerY + HEADER_H * 0.25, TITLE_FONT_SZ, COL_TEXT_TITLE)

    -- Close button
    local closeBtnW = 44.0
    local closeBtnX = panelX + PANEL_W - closeBtnW - 6
    local closeBtnY = headerY + 3
    local closeBtnH = HEADER_H - 6
    local closeHover = pointInRect(mx, my, closeBtnX, closeBtnY, closeBtnW, closeBtnH)
    drawRect(closeBtnX, closeBtnY, closeBtnW, closeBtnH,
             closeHover and COL_CLOSE_HOVER or COL_CLOSE_BTN)
    drawText("X", closeBtnX + 14, closeBtnY + closeBtnH * 0.22, ITEM_FONT_SZ + 2, COL_BTN_TEXT)
    btnClose = { x = closeBtnX, y = closeBtnY, w = closeBtnW, h = closeBtnH }

    -- Info / description area
    local infoY = panelY
    drawRect(panelX, infoY, PANEL_W, INFO_H, COL_INFO_BG)

    if selectedId and items[selectedId] then
        local d = items[selectedId]
        drawText(d.name,  panelX + 12, infoY + INFO_H - 28, INFO_FONT_SZ,     COL_TEXT_TITLE)
        drawText("x" .. d.qty, panelX + 12, infoY + INFO_H - 52, ITEM_FONT_SZ, COL_TEXT_QTY)

        local btnW, btnH = 90.0, 30.0
        local btnY = infoY + 8
        btnUse  = makeButton("USE",  panelX + PANEL_W - btnW * 2 - 20, btnY, btnW, btnH,
                             COL_BTN_NORMAL, COL_BTN_HOVER, mx, my)
        btnDrop = makeButton("DROP", panelX + PANEL_W - btnW - 10,     btnY, btnW, btnH,
                             COL_CLOSE_BTN, COL_CLOSE_HOVER, mx, my)
    else
        drawText("Select an item to inspect it.",
                 panelX + 12, infoY + INFO_H * 0.45, ITEM_FONT_SZ, COL_TEXT_DESC)
        btnUse  = nil
        btnDrop = nil
    end

    if dragIdx then
        drawText("Drop onto another item to combine",
                 panelX + 12, infoY + 8, ITEM_FONT_SZ - 2, COL_COMBINE_HINT)
    end

    if combineFlashTimer > 0 then
        drawText(combineFlashMsg, panelX + 12, infoY + 8, ITEM_FONT_SZ - 2, COL_COMBINE_HINT)
    end

    -- Text labels drawn on top of each slot entity
    for i = 1, MAX_SLOTS do
        local sx, sy = slotLocalPos(i, panelX, panelY)
        if sy + SLOT_SIZE > panelY + PANEL_H - HEADER_H then break end

        local itemId, itemData = getItemAt(i)
        local isDragSrc = (dragIdx == i)

        if itemData and not isDragSrc then
            local shortName = truncate(itemData.name, 9)
            drawText(shortName,        sx + 6, sy + SLOT_SIZE - 22, ITEM_FONT_SZ - 2, COL_TEXT_ITEM)
            drawText("x" .. itemData.qty, sx + 6, sy + 6,           ITEM_FONT_SZ - 2, COL_TEXT_QTY)
        elseif not itemData then
            drawText(tostring(i), sx + SLOT_SIZE * 0.4, sy + SLOT_SIZE * 0.4,
                     ITEM_FONT_SZ, { 0.25, 0.25, 0.28, 1.0 })
        end
    end

    -- Drag ghost: show item name + qty floating at cursor
    if dragIdx then
        local itemId, itemData = getItemAt(dragIdx)
        if itemData then
            local gx = dragX - SLOT_SIZE * 0.5
            local gy = dragY - SLOT_SIZE * 0.5
            drawRect(gx, gy, SLOT_SIZE, SLOT_SIZE, { 0.35, 0.55, 0.80, 0.70 })
            drawText(truncate(itemData.name, 9),   gx + 6, gy + SLOT_SIZE - 22, ITEM_FONT_SZ - 2, COL_TEXT_ITEM)
            drawText("x" .. itemData.qty, gx + 6, gy + 6, ITEM_FONT_SZ - 2, COL_TEXT_QTY)
        end
    end
end

----------------------------------------------------------------
-- on_create: build slot entity pool
----------------------------------------------------------------
function on_create(Self)
    UI = EntityUI
    if not UI then
        print("[ui_inventory] WARNING: No UIComponent on entity")
    end
    if UI then UI:SetVisible(false) end

    -- Slot entities are expected to be pre-created in the editor as
    -- plain UIComponent entities with no scene mesh. We look them up
    -- from SHARED.UI.SlotEntities if the level populates that table,
    -- OR we fall back to an inline creation table you can fill below.
    --
    -- Pattern A (editor-driven, recommended):
    --   In another script or level setup, populate:
    --       SHARED.UI = SHARED.UI or {}
    --       SHARED.UI.InventorySlotEntities = { entityId1, entityId2, ... }
    --   before this script's on_create runs.
    --
    -- Pattern B (manual): fill slotEntities directly with entity IDs
    --   that already exist in the registry (editor-placed).
    --       slotEntities = { 42, 43, 44, ... }

    local S = getShared()
    if S and S.UI and S.UI.InventorySlotEntities then
        slotEntities = S.UI.InventorySlotEntities
        print("[ui_inventory] Loaded", #slotEntities, "slot entities from SHARED.UI.InventorySlotEntities")
    else
        -- No slot entities found yet; updateSlotEntities() will be a no-op
        -- until they are registered. Call the above setup from your level script.
        print("[ui_inventory] WARNING: SHARED.UI.InventorySlotEntities not set. " ..
              "Slot textures will not display until slot entities are registered.")
    end

    print("[ui_inventory] Initialized")
end

----------------------------------------------------------------
-- on_update
----------------------------------------------------------------
function on_update(Self, dt)
    -- Inventory toggle
    local invKeyDown = Input.GetKeyDown(66) -- B key (GLFW 66)
    local S = getShared()

    if invKeyDown and not prevInvKey then
        isOpen = not isOpen
        if UI then UI:SetVisible(isOpen) end
        if isOpen then
            Input.SetCursorVisible(true)
            Input.SetCursorLocked(false)
            refreshSlotOrder()
            selectedId = nil
            dragIdx    = nil
        else
            Input.SetCursorVisible(false)
            Input.SetCursorLocked(true)
            -- Hide all slot entities when closing
            for _, eid in ipairs(slotEntities) do
                local slotUI = GetEntityUI(eid)
                if slotUI then slotUI:SetVisible(false) end
            end
        end
        print("[ui_inventory] Toggled:", isOpen and "OPEN" or "CLOSED")
    end
    prevInvKey = invKeyDown

    if combineFlashTimer > 0 then
        combineFlashTimer = combineFlashTimer - dt
    end

    if not isOpen then return end

    -- Mouse position in virtual-UI coords
    local rawMx, rawMy = Input.GetMousePosition()
    local mx, my = toUICoords(rawMx, rawMy)

    hoveredIdx = slotIndexAt(mx, my)

    local mouseDown = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local mouseHeld = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local mouseUp   = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)

    if dragIdx and mouseHeld then
        dragX = mx
        dragY = my
    end

    -- Mouse DOWN
    if mouseDown and not prevMouseDown then
        if btnClose and pointInRect(mx, my, btnClose.x, btnClose.y, btnClose.w, btnClose.h) then
            isOpen = false
            if UI then UI:SetVisible(false) end
            Input.SetCursorVisible(false)
            Input.SetCursorLocked(true)
            for _, eid in ipairs(slotEntities) do
                local slotUI = GetEntityUI(eid)
                if slotUI then slotUI:SetVisible(false) end
            end
            dragIdx = nil; selectedId = nil
            prevMouseDown = mouseDown; return
        end

        if btnUse and pointInRect(mx, my, btnUse.x, btnUse.y, btnUse.w, btnUse.h) then
            if selectedId then
                print("[ui_inventory] USE:", selectedId)
                if S and S.Game and S.Game.UseItem then S.Game.UseItem(selectedId) end
            end
            prevMouseDown = mouseDown; return
        end

        if btnDrop and pointInRect(mx, my, btnDrop.x, btnDrop.y, btnDrop.w, btnDrop.h) then
            if selectedId then
                print("[ui_inventory] DROP:", selectedId)
                local inv = getInventory()
                if inv then
                    inv.Remove(selectedId, 1)
                    if inv.Count(selectedId) <= 0 then selectedId = nil end
                end
            end
            prevMouseDown = mouseDown; return
        end

        if hoveredIdx then
            local itemId = getItemAt(hoveredIdx)
            if itemId then
                dragIdx    = hoveredIdx
                dragX      = mx
                dragY      = my
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

    -- Mouse UP
    if mouseUp and not prevMouseUp then
        if dragIdx then
            local dropIdx = slotIndexAt(mx, my)
            if dropIdx and dropIdx ~= dragIdx then
                local srcId = getItemAt(dragIdx)
                local tgtId = getItemAt(dropIdx)
                if srcId and tgtId then
                    local inv = getInventory()
                    if inv and inv.GetRecipe then
                        local recipe = inv.GetRecipe(srcId, tgtId)
                        if recipe then
                            combineFlashMsg   = "Combining: " .. srcId .. " + " .. tgtId .. " => " .. recipe.name
                            combineFlashTimer = 2.5
                            local ok = inv.Combine and inv.Combine(srcId, tgtId)
                            if ok then
                                print("[ui_inventory] Combine SUCCESS =>", recipe.name)
                                selectedId = recipe.id
                            end
                        else
                            combineFlashMsg   = "No recipe for " .. srcId .. " + " .. tgtId
                            combineFlashTimer = 1.5
                        end
                    end
                end
            elseif dropIdx == dragIdx then
                local itemId = getItemAt(dragIdx)
                selectedId = itemId or selectedId
            end
            dragIdx = nil
        end
    end

    prevMouseDown = mouseDown
    prevMouseUp   = mouseUp

    -- Update inventory slot order for this frame
    refreshSlotOrder()

    -- Sync slot UIComponent entities (texture + position + visibility)
    updateSlotEntities()

    -- Draw panel overlay (header, info bar, text labels)
    render(mx, my)
end

----------------------------------------------------------------
-- on_destroy
----------------------------------------------------------------
function on_destroy(Self)
    if isOpen then
        Input.SetCursorVisible(false)
        Input.SetCursorLocked(true)
    end
    -- Hide all slot entities on cleanup
    for _, eid in ipairs(slotEntities) do
        local slotUI = GetEntityUI(eid)
        if slotUI then slotUI:SetVisible(false) end
    end
    print("[ui_inventory] Destroyed")
end