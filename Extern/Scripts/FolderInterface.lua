----------------------------------------------------------------
-- FolderInterface.lua
-- Attach to the ROOT UI entity inside menu "FolderInterface".
-- Must have a UIComponent (EntityUI).
--
-- Left-click (Windows) / SHARED.FOLDER_NEXT = true (Android) advances pages.
-- E key closes the folder from anywhere (consumed here directly).
-- Page visibility is handled per-page by FolderPage.lua.
-- Layer / position offsets for the peek effect are set in the scene.
----------------------------------------------------------------

local FOLDER_ID = 764       -- <<< world folder entity ID (SF_Evidence_Folder)
local UI_TABLE  = "FOLDER_UI_OPEN"
local NUM_PAGES = 4

local UI          = nil
local uiWasOpen   = nil      -- nil = uninitialized
local currentPage = 1
local prevClick   = false
local prevEDown   = false
local clickCooldown = 0.0
local CLICK_COOLDOWN = 0.25  -- seconds between page advances

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
    if SHARED[UI_TABLE][FOLDER_ID] == nil then
        SHARED[UI_TABLE][FOLDER_ID] = false
    end
    SHARED.FOLDER_PAGE = 0

    UI:SetVisible(false)
end

function on_update(self, dt)
    if not UI then return end

    local open = want_open()
    UI:SetVisible(open)

    -- First update: initialize state without touching cursor
    if uiWasOpen == nil then
        uiWasOpen = open
        return
    end

    -- Open transition
    if open and not uiWasOpen then
        set_cursor(true)
        currentPage = 1
        publish_page()
        prevClick = false
        prevEDown = false
        clickCooldown = CLICK_COOLDOWN  -- prevent accidental click on the frame of opening
        print("[FolderInterface] Opened, page 1")
    end

    -- Close transition: don't force-hide cursor, camera's hold-to-look handles it
    uiWasOpen = open

    if not open then return end

    ----------------------------------------------------------------
    -- E key: close folder directly (works regardless of range)
    ----------------------------------------------------------------
    local uiShared = SHARED.UI
    if uiShared and uiShared.actionRequested then
        uiShared.actionRequested = false
        close_folder()
        print("[FolderInterface] Closed via E")
        return
    end

    ----------------------------------------------------------------
    -- Advance page: J key (Windows) or SHARED.FOLDER_NEXT (Android)
    ----------------------------------------------------------------
    local clicked = false

    if PLATFORM == "windows" then
        local isDown = Input.GetMouseButton and Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT) or false
        if isDown and not prevClick then clicked = true end
        prevClick = isDown
    elseif PLATFORM == "android" then
        if SHARED.FOLDER_NEXT then
            SHARED.FOLDER_NEXT = false
            clicked = true
        end
    end

    -- Tick cooldown
    if clickCooldown > 0 then
        clickCooldown = clickCooldown - dt
    end

    if clicked and clickCooldown <= 0 then
        if currentPage < NUM_PAGES then
            currentPage = currentPage + 1
            publish_page()
            clickCooldown = CLICK_COOLDOWN
            print("[FolderInterface] Page ->", currentPage)
        else
            close_folder()
            print("[FolderInterface] Last page clicked, closing")
        end
    end
end
