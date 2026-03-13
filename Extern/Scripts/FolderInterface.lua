----------------------------------------------------------------
-- FolderInterface.lua
-- Attach to the ROOT UI entity inside menu "FolderInterface".
-- Must have a UIComponent (EntityUI).
--
-- E key / Android action input advances pages.
-- When the last page is flipped, the folder closes and triggers the girl.
----------------------------------------------------------------

local FOLDER_ID = 764
local UI_TABLE  = "FOLDER_UI_OPEN"
local NUM_PAGES = 4

local UI            = nil
local uiWasOpen     = nil
local currentPage   = 1
local clickCooldown = 0.0
local CLICK_COOLDOWN = 0.25

local function want_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][FOLDER_ID] == true
end

local function set_cursor(open)
    if Input and Input.SetCursorVisible then Input.SetCursorVisible(open) end
    if Input and Input.SetCursorLocked  then Input.SetCursorLocked(not open) end
end

local function publish_page()
    SHARED.FOLDER_PAGE = currentPage
end

local function close_folder()
    SHARED[UI_TABLE][FOLDER_ID] = false
    SHARED.FOLDER_PAGE = 0
    if UI_CloseAllMenus ~= nil then UI_CloseAllMenus() end
    if PLATFORM == "android" then
        UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
        UI_DisplayMenu("AndroidInputOverlay")
    end
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[FolderInterface] ERROR: No UIComponent on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED.UI = SHARED.UI or {}

    if SHARED[UI_TABLE][FOLDER_ID] == nil then
        SHARED[UI_TABLE][FOLDER_ID] = false
    end

    SHARED.FOLDER_PAGE = 0
    SHARED.EVIDENCE_FOLDER_FINISHED = false

    UI:SetVisible(false)
end

function on_update(self, dt)
    if not UI then return end
    dt = dt or 0.016

    local open = want_open()
    UI:SetVisible(open)

    if uiWasOpen == nil then
        uiWasOpen = open
        return
    end

    if open and not uiWasOpen then
        set_cursor(true)
        currentPage = 1
        publish_page()
        clickCooldown = CLICK_COOLDOWN
        print("[FolderInterface] Opened, page 1")
    end

    uiWasOpen = open

    if not open then return end

    if clickCooldown > 0 then
        clickCooldown = clickCooldown - dt
    end

    ------------------------------------------------------------
    -- ONLY actionRequested flips pages
    -- Windows: E key
    -- Android: action input button
    ------------------------------------------------------------
    local uiShared = SHARED.UI
    local advanceRequested = false

    if uiShared and uiShared.actionRequested then
        uiShared.actionRequested = false
        advanceRequested = true
    end

    if not advanceRequested or clickCooldown > 0 then
        return
    end

    if currentPage < NUM_PAGES then
        currentPage = currentPage + 1
        publish_page()
        clickCooldown = CLICK_COOLDOWN
        print("[FolderInterface] Page ->", currentPage)
    else
        SHARED.EVIDENCE_FOLDER_FINISHED = true
        close_folder()
        print("[FolderInterface] Last page confirmed -> trigger girl")
    end
end