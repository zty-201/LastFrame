/*
*****************************************************************/
/*!
\file   Level_Editor.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\co-author Tang Yeun, Dean
\par    y.tang\@digipen.edu
\date   September 2025
\brief
This file contains the main loop of level editor
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/


#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)


#include "pch.hpp"
#include "global.hpp"
#include "LFeature.hpp"
//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

//Entt reflection system
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
//Entt ECS system
#include <entt/entity/registry.hpp>
#include "System_LevelEditor.hpp"
#include "Level_Editor.hpp"

//Casting meta data to type.
#include "ReflectionRegister.hpp"
#include "GraphicsSystem.h"
#include "ObjectManager.hpp"
#include "Save.hpp"
#include "Screen.hpp"
#include "SceneManager.hpp"
#include "AssetWindow.hpp"
#include "ModelManager.hpp"
#include "AssetManager.hpp"
#include "GameplayKeyManager.hpp"
// /Debug
#include "Profiler.hpp"

// Script hot reloading
#include "Script.hpp" // Engine::ScriptSystem::RequestReloadAll / RequestReloadEntity
#include "MenuManager.hpp"
#include "PropertyManager.hpp"
#include "GameplayKeyManager.hpp"
#include "UISystem.hpp"
namespace
{
constexpr const char* kDefaultLayoutRelativePath = "Extern/LevelEditor/imgui_default.ini";
}

namespace EngineFramework
{
	bool LevelEditor::is_enabled = true;

	LevelEditor::LevelEditor()
	{
		this_system = this;
		is_enabled = false; //by default it should be set to false

		//to_open = false;
		to_close = false;

		variables.selected_modelID = {}; //zero means none selected
		variables.ClearEntitySelection();
		variables.latest_selected = iEditorView::Latest_Selected::NONE;
		LOGI("LevelEditor() constructor");
	}
	LevelEditor::~LevelEditor() {

		//iff the window is closed while level editor is running, rmb to clean
		if (is_enabled) {
			Clean();
		}	
		LOGI("~LevelEditor() destructor");
	}

	void LevelEditor::addFeature(const std::string& feature_name, std::unique_ptr<LFeature> feature_class, bool enabled) {

		feature_class->set_Pointer(&variables);
		feature_class->is_enabled = enabled;
		featureMap[feature_name] = std::move(feature_class);

	}
	const bool& LevelEditor::featureFlag(const std::string& name) const {
		auto it = featureMap.find(name);
		if (it == featureMap.end()) throw std::out_of_range("No such feature: " + name);
		return it->second->is_enabled;
	}

	bool& LevelEditor::featureFlag(const std::string& feature_name) {
		auto it = featureMap.find(feature_name);

		if (it == featureMap.end()) {
			throw std::out_of_range("No such feature: " + feature_name);
		}
		return it->second->is_enabled;
	}




	void LevelEditor::Initialize()
	{
		

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigWindowsResizeFromEdges = true; // allow resize from any edge
		ImGui::GetIO().ConfigDockingTransparentPayload = false;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.ConfigWindowsMoveFromTitleBarOnly = true;

		//Enable docking of imgui windows.
		std::filesystem::path ini_path;
		auto try_candidate = [&ini_path](const std::filesystem::path& candidate)
		{
			if (ini_path.empty() && std::filesystem::exists(candidate))
			{
				ini_path = std::filesystem::weakly_canonical(candidate);
			}
		};

		const auto cwd = std::filesystem::current_path();
		LOGI("LevelEditor current working directory: %s", cwd.string().c_str());
		try_candidate(cwd / "imgui.ini");
		try_candidate(cwd / "build/VisualStudio/imgui.ini");
		try_candidate(cwd / "build/imgui.ini");
		try_candidate(cwd / "../build/VisualStudio/imgui.ini");

		for (auto path = cwd; ini_path.empty(); )
		{
			try_candidate(path / "imgui.ini");
			try_candidate(path / "build/VisualStudio/imgui.ini");

			if (!path.has_parent_path() || path == path.parent_path())
			{
				break;
			}
			path = path.parent_path();
		}

		if (ini_path.empty())
		{
			ini_path = (cwd / "imgui.ini").lexically_normal();
		}

		static std::string ini_path_string = ini_path.string(); // persist for ImGui lifetime
		io.IniFilename = ini_path_string.c_str();

		if (std::filesystem::exists(ini_path))
		{
			ImGui::LoadIniSettingsFromDisk(io.IniFilename);
			LOGI("Loaded ImGui layout from: %s", io.IniFilename);
		}
		else
		{
			std::filesystem::path default_ini_path;
			auto try_default = [&default_ini_path](const std::filesystem::path& base)
			{
				if (default_ini_path.empty())
				{
					const auto candidate = base / kDefaultLayoutRelativePath;
					if (std::filesystem::exists(candidate))
					{
						default_ini_path = std::filesystem::weakly_canonical(candidate);
					}
				}
			};

			try_default(cwd);
			for (auto path = cwd; default_ini_path.empty(); )
			{
				try_default(path);
				if (!path.has_parent_path() || path == path.parent_path())
				{
					break;
				}
				path = path.parent_path();
			}

			if (!default_ini_path.empty())
			{
				std::ifstream default_file(default_ini_path, std::ios::binary);
				if (default_file)
				{
					std::ostringstream buffer;
					buffer << default_file.rdbuf();
					const std::string contents = buffer.str();
					if (!contents.empty())
					{
						ImGui::LoadIniSettingsFromMemory(contents.c_str(), contents.size());
						ImGui::SaveIniSettingsToDisk(io.IniFilename);
						LOGI("Created default ImGui layout at: %s (template: %s)", io.IniFilename, default_ini_path.string().c_str());
					}
					else
					{
						LOGE("Default ImGui layout template was empty: %s", default_ini_path.string().c_str());
					}
				}
				else
				{
					LOGE("Failed to read default ImGui layout template: %s", default_ini_path.string().c_str());
				}
			}
			else
			{
				LOGE("Default ImGui layout template not found relative to working directory; expected path %s", kDefaultLayoutRelativePath);
			}
		}
		
		// Register reflection (TESTING PURPOSES)
		EngineFramework::RegisterReflection();

		auto cam = entt::resolve<CameraComponent>();
		const size_t camFields = (size_t)std::distance(cam.data().begin(), cam.data().end());
		printf("[Reflection] CameraComponent fields registered: %zu\n", camFields);
		
		/*
		 Check if ImGui needs the keyboard. If so, disable in-game key input.
		 e.g. Typing into an imgui text field shouldn't move the player.
		*/
		//Clicking on an imgui window will automatically cause checking of keyboard input, for imgui keyboard navigation.
		//Disable this flag to make the next check possible even when in an imgui window
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

		// Initialize the ImGui backend for your existing OpenGL context
		ImGui_ImplOpenGL3_Init("#version 300 es");

		imgui_flags.DockWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		imgui_flags.mainWindowFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDocking;   // <- prevents docking

		// If using GLFW for input, you can still initialize the input handling, but use your custom window
		if (GraphicsSystem::GetInstance().GetGLFWwindow() == nullptr) {
			LOGI("level editor app cannot be initialised");
			throw std::runtime_error("Level editor init failed: GraphicsSystem has no valid GLFWwindow.");
		}
		else {
			ImGui_ImplGlfw_InitForOpenGL(GraphicsSystem::GetInstance().GetGLFWwindow(), true);
			LOGI("level editor app is initialised");
		}

		//FEATURES ===============================================================

		/*add the following submenu here*/
		addFeature("ObjectManager", std::make_unique<ObjectManager>(), true);
		addFeature("Save", std::make_unique<Save>());
		addFeature("Screen", std::make_unique<Screen>(), true);
		addFeature("Profiler", std::make_unique<LFeature_DebugProfiler>(), true); // Add profiler feature (testing purpose)
		
		addFeature("SceneManager", std::make_unique<SceneManager>()); //
		addFeature("AssetWindow", std::make_unique<AssetWindow>(), true);//
		addFeature("ModelManager", std::make_unique<ModelManager>());//
		addFeature("Gameplay_KeyManager", std::make_unique<Gameplay_KeyManager>());
		addFeature("PropertyManager", std::make_unique<LFeature_PropertyManager>(), true);
		addFeature("MenuManager", std::make_unique<MenuManager>());
		//

		SceneManager::GetInstance().ScanScenes(AssetManagerSystem::GetAssetFolderPrefix().string() + "Extern/Scenes");//

		/*initialise all the features*/
		for (auto& [name, feature] : featureMap) {

			feature->Initialize();			
		}

	}

	void LevelEditor::Open()
	{
		is_enabled = true;
		LOGI("app opened"); //okay good check
	}

	void LevelEditor::docking() {

		ImGuiIO& io = ImGui::GetIO();
		float headerHeight = 30.f;
		// Create a full-screen root dockspace window for all other windows (custom docking)
		ImGui::SetNextWindowPos(ImVec2(0, headerHeight), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - headerHeight), ImGuiCond_Always);

		if (ImGui::Begin("Main Dockspace", nullptr, imgui_flags.DockWindowFlags)) {
			// Create a central dockspace
			ImGuiID dockspace_id = ImGui::GetID("Main Dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		}
		ImGui::End();
	}

	void LevelEditor::main_window() {

		//MAIN MENU SHOULD CONTAIN THE SUB MENUS

		ImGuiIO& io = ImGui::GetIO();
		
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar

		// Set the window at the top of the screen, full width, small height
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 30), ImGuiCond_Always); // 30 px height for header

		
		if (ImGui::Begin("Main Window", nullptr, imgui_flags.mainWindowFlags)) {

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text

			if (ImGui::BeginMenuBar()) {

				if (ImGui::BeginMenu("File")) {

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));      // Black text
					//std::cout << "File clicked\n";

					if (ImGui::BeginMenu("Create New")) {
						ImGui::MenuItem("Level", NULL);
						ImGui::EndMenu();
					}

					/*ImGui::MenuItem("Open Existing", NULL, &featureFlag("Screen"));
					ImGui::Separator();*/
					ImGui::MenuItem("Open Scene", NULL, &featureFlag("SceneManager"));
					ImGui::Separator();
					ImGui::MenuItem("Menu Manager", NULL, &featureFlag("MenuManager"));
					ImGui::Separator();

					if (ImGui::MenuItem("Save", NULL)) {
						Save::GetInstance().SAVEFILE(SceneManager::GetInstance().GetCurrentScene());
					}

					ImGui::MenuItem("Save As ..", NULL, &featureFlag("Save"));
					ImGui::Separator(); 
					ImGui::MenuItem("Save & Quit", NULL);
					ImGui::MenuItem("Quit", NULL, &to_close);
					ImGui::PopStyleColor();  // Pop the white text color for "File" menu
					ImGui::EndMenu();

				}

				ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


				if (ImGui::BeginMenu("Manage")) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));

					ImGui::MenuItem("Model Manager", nullptr, &featureFlag("ModelManager"));
					ImGui::Separator();
					ImGui::MenuItem("Object Manager", nullptr, &featureFlag("ObjectManager"));

					ImGui::Separator();
					if (ImGui::BeginMenu("Property Manager")) {
						if (auto it = featureMap.find("PropertyManager"); it != featureMap.end()) {
							bool& enabled = it->second->is_enabled;
							if (ImGui::MenuItem("Show Properties", nullptr, enabled)) {
								enabled = !enabled;
							}
						}
						else {
							ImGui::TextDisabled("Feature not registered");
						}
						ImGui::EndMenu(); 
					}

					ImGui::Separator();
					ImGui::MenuItem("Asset Browser", nullptr, &featureFlag("AssetWindow"));
					ImGui::Separator();
					ImGui::MenuItem("Input Key Manager", NULL, &featureFlag("Gameplay_KeyManager"));
					ImGui::Separator();
					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels

				// Debug menu
				if (ImGui::BeginMenu("Debug")) {
					// Single toggle that controls the profiler window visibility
					if (auto it = featureMap.find("Profiler"); it != featureMap.end()) {
						auto* prof = static_cast<LFeature_DebugProfiler*>(it->second.get());
						bool on = prof->is_enabled; // Panel visibility flag
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
						if (ImGui::MenuItem("Profiler", nullptr, &on)) {
							prof->is_enabled = on; // Show/hide panel
							//it->second->is_enabled = true; // Keep feature running
						}
						ImGui::PopStyleColor();
					}

					auto& gfx = EngineFramework::GraphicsSystem::GetInstance();

					bool draw = gfx.GetDebugStatus();

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
					if (ImGui::Checkbox("Show Debug Wireframe", &draw)) {
						gfx.SetDebugStatus(draw);
					}

					ImGui::PopStyleColor();

					ImGui::Separator();

					// Reload ALL Lua scripts (safe queued request)
					if (ImGui::MenuItem("Reload ALL Lua Scripts")) {
						Engine::ScriptSystem::RequestReloadAll();
					}

					// Reload selected entity's Lua scripts (safe queued request)
					if (variables.selected_objectID != entt::null) {
						if (ImGui::MenuItem("Reload Selected Entity Lua Scripts")) {
							Engine::ScriptSystem::RequestReloadEntity(variables.selected_objectID);
						}
					}

					ImGui::EndMenu();
				}

				
				//if (ImGui::BeginMenu("Property Manager")) {
				//	bool* enabledPtr = nullptr;

				//	if (auto it = featureMap.find("PropertyManager"); it != featureMap.end()) {
				//		enabledPtr = &it->second->is_enabled;               // use base pointer, no cast
				//	}

				//	bool on = enabledPtr ? *enabledPtr : false;

				//	// If feature not found, item is disabled and shows (so you see text)
				//	if (ImGui::MenuItem("Show Properties", nullptr,
				//		enabledPtr ? &on : nullptr,          // checkbox only if feature exists
				//		enabledPtr != nullptr))              // enabled/disabled state
				//	{
				//		*enabledPtr = on;                                    // toggle when present
				//	}

				//	if (!enabledPtr) {
				//		ImGui::TextDisabled("Feature not registered");
				//	}

				//	ImGui::EndMenu();
				//}


				ImGui::SameLine(0, 50.f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				const std::string& currScene = SceneManager::GetInstance().GetCurrentScene();
				ImGui::Text("| Current Scene: %s", currScene.empty() ? "None" : currScene.c_str());
				ImGui::PopStyleColor();

				ImGui::EndMenuBar();
			}

			ImGui::PopStyleColor();
		}


		ImGui::PopStyleColor();

		ImGui::End();


		//iterate through the featureMap and runs active subwindows/function
		for (auto& [name, feature] : featureMap) {
			try
			{
				if (feature->is_enabled) {
					feature->Run();
				}
			}
			catch (std::exception& e)
			{
				LOGE("Exception in Level_Editor::main_window() Feature %s: %s", name.c_str(), e.what());
				continue;
			}
		}

	}

	void LevelEditor::Run()
	{
		//1. Level Editor Update loop should run after Graphics System Update Loop, to allow the scene to draw to framebuffer.
		
		// 2. Now that the FBO is populated, we can prepare to draw the UI.
		// First, clear the MAIN WINDOW to a background color. This gives ImGui a clean slate to draw on.
		// We bind framebuffer 0 (the screen) just to be explicit.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // A dark background for the editor
		glClear(GL_COLOR_BUFFER_BIT); // We only need to clear the color buffer for the UI


		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::GetIO().FontGlobalScale = 2.0f;

		//Disable keyboard if it's being used by imgui, like an imgui text field being active.
		if (ImGui::GetIO().WantCaptureKeyboard && ImGui::IsAnyItemActive())
		{
			//UserInterface_System::DisableKeyboardInput();
		}
		ImGuiIO& io = ImGui::GetIO();
		//ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = 1.0f;   // DEFAULT

		//DOCKING =================================================		
		docking();
		//MAIN MENU ===============================================
		main_window();
		//RENDERING ===============================================
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

	void LevelEditor::Close()
	{
		//for item such as pointer or stack should be released, no point for editor to be holding it
		for (auto& [name, feature] : featureMap) {	
			feature->Close();		
		}
		
		to_close = false; //after closing, set this to false.
		is_enabled = false;
	}

	void LevelEditor::Clean() {

		//when the game is exited, we should clean this up to prevent mem leak
		ImGui_ImplOpenGL3_Shutdown();  // OpenGL backend
		ImGui_ImplGlfw_Shutdown();     // GLFW backend
		ImGui::DestroyContext();
		LOGI("cleaned");
	}

	

}
#endif

