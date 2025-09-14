#ifdef _DEBUG

#include "pch.hpp"
#include "global.hpp"
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

namespace EngineFramework
{
	bool LevelEditor::is_enabled = true;

	LevelEditor::LevelEditor()
	{
		this_system = this;
		is_enabled = true; //by default it should be set to false

		to_open = false;
		to_close = false;

		variables.selected_modelID = 0; //zero means none selected
		variables.selected_objectID = 0; //zero means none selected

		std::cout << "LevelEditor() constructor\n";
	}
	LevelEditor::~LevelEditor() {

		ImGui::DestroyContext();
		std::cout << "~LevelEditor() destructor\n";
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

		/*
		 Check if ImGui needs the keyboard. If so, disable in-game key input.
		 e.g. Typing into an imgui text field shouldn't move the player.
		*/
		//Clicking on an imgui window will automatically cause checking of keyboard input, for imgui keyboard navigation.
		//Disable this flag to make the next check possible even when in an imgui window
		ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

		// Initialize the ImGui backend for your existing OpenGL context
		ImGui_ImplOpenGL3_Init("#version 450");

		imgui_flags.DockWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;

		// If using GLFW for input, you can still initialize the input handling, but use your custom window
		if (g_graphics_ptr->GetGLFWwindow() == nullptr) {
			std::cout << "level editor app cannot be initialised\n";
			throw std::runtime_error("Level editor init failed: GraphicsSystem has no valid GLFWwindow.");
		}
		else {
			ImGui_ImplGlfw_InitForOpenGL(g_graphics_ptr->GetGLFWwindow(), true);
			std::cout << "level editor app is initialised\n";
		}

		//FEATURES ===============================================================

		/*add the following submenu here*/
		addFeature("ObjectManager", std::make_unique<ObjectManager>());
		addFeature("Save", std::make_unique<Save>());

		/*initialise all the features*/
		for (auto& [name, feature] : featureMap) {

			if (feature->is_enabled) {
				feature->Initialize();
			}
		}

	}

	void LevelEditor::Open()
	{
		to_open = false; //after open, set this to false.
		std::cout << "app opened\n"; //okay good check
	}

	void LevelEditor::docking() {

		// Create a full-screen root dockspace window for all other windows (custom docking)
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);

		if (ImGui::Begin("Main Dockspace", nullptr, imgui_flags.DockWindowFlags)) {
			// Create a central dockspace
			ImGuiID dockspace_id = ImGui::GetID("Main Dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		}
		ImGui::End();
	}

	void LevelEditor::main_window() {

		//MAIN MENU SHOULD CONTAIN THE SUB MENUS

		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White bar
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Once);  // Width: 300,500

		if (ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar)) {

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));      // Black text

			if (ImGui::BeginMenuBar()) {

				if (ImGui::BeginMenu("File")) {

					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White text for "File"
					//std::cout << "File clicked\n";

					if (ImGui::BeginMenu("Create New")) {
						ImGui::MenuItem("Level", NULL);
						ImGui::EndMenu();
					}

					ImGui::MenuItem("Open Existing", NULL);
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
					ImGui::PopStyleColor();  // Pop the white text color for "File" menu
					ImGui::EndMenu();
				}

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
		//std::cout << "yo!\n";
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
		ImGui_ImplOpenGL3_Shutdown();  // OpenGL backend
		ImGui_ImplGlfw_Shutdown();     // GLFW backend
		ImGui::DestroyContext();
		to_close = false; //after closing, set this to false.
		is_enabled = false;
	}

	void LevelEditor::Clean() {

		//when the game is exited, we should clean this up to prevent mem leak
	}

	void LevelEditor::Toggle() {

		if (is_enabled) {
			//if is_enabled = true, then we toggle, so we are closing now
			to_close = true;
		}
		else {
			//if is_enabled = false, then we toggle, so we are opening now
			to_open = true;
		}

		is_enabled = !is_enabled;
	}

}
#endif


