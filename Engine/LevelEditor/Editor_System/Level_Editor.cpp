#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)


#include "pch.hpp"
#include "global.hpp"
#include "LFeature.hpp"
//File management, JSON file parser/writer
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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

// Profiler/Debug
#include "Profiler.hpp"

namespace EngineFramework
{
	bool LevelEditor::is_enabled = true;

	LevelEditor::LevelEditor()
	{
		this_system = this;
		is_enabled = false; //by default it should be set to false

		//to_open = false;
		to_close = false;

		variables.selected_modelID = 0; //zero means none selected
		variables.selected_objectID = 0; //zero means none selected

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

		//Enable docking of imgui windows.
		ImGui::LoadIniSettingsFromMemory(""); //reset any docking layout from memory, please comment this out to enable docking config memory
		ImGui::GetIO().ConfigDockingTransparentPayload = false;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.ConfigWindowsMoveFromTitleBarOnly = true;


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
		addFeature("ObjectManager", std::make_unique<ObjectManager>());
		addFeature("Save", std::make_unique<Save>());
		addFeature("Screen", std::make_unique<Screen>());
		addFeature("Profiler", std::make_unique<LFeature_DebugProfiler>()); // Add profiler feature (testing purpose)
		
		addFeature("SceneManager", std::make_unique<SceneManager>());
		addFeature("AssetWindow", std::make_unique<AssetWindow>());

		SceneManager::GetInstance().ScanScenes("Extern/Scenes");//

		/*initialise all the features*/
		for (auto& [name, feature] : featureMap) {

			if (feature->is_enabled) {
				feature->Initialize();
			}
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

					ImGui::MenuItem("Open Existing", NULL, &featureFlag("Screen"));
					ImGui::Separator();
					ImGui::MenuItem("Open Scene", NULL, &featureFlag("SceneManager"));
					ImGui::Separator();
					ImGui::MenuItem("Save", NULL, &featureFlag("Save"));
					ImGui::MenuItem("Save As ..", NULL);
					ImGui::Separator(); 
					ImGui::MenuItem("Save & Quit", NULL);
					ImGui::MenuItem("Quit", NULL, &to_close);
					ImGui::PopStyleColor();  // Pop the white text color for "File" menu
					ImGui::EndMenu();

				}

				ImGui::SameLine(0, 50.0f); // The second argument controls the spacing in pixels


				if (ImGui::BeginMenu("Manage")) {

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));      // White text

					ImGui::MenuItem("Model Manager", NULL);
					ImGui::Separator();
					ImGui::MenuItem("Object Manager", NULL, &featureFlag("ObjectManager"));
					ImGui::Separator();
					ImGui::MenuItem("Property Manager", NULL);
					ImGui::Separator();
					ImGui::MenuItem("Asset Browser", NULL, &featureFlag("AssetWindow"));
					ImGui::PopStyleColor();  // Pop the white text color for "File" menu
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

					ImGui::EndMenu();
				}

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

			if (feature->is_enabled) {
				feature->Run();
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

