/*
	The backbone of the game architecture, it runs in main() and calls all systems attached to it, in order.
*/

#ifndef CORE_ENGINE_HPP
#define CORE_ENGINE_HPP
#include <string>
#include <memory>
#include <vector>
#include "ISystem.hpp"
namespace EngineFramework
{
	class ISystem;
	/*
		Manages all game systems, init/running/destroying them in order.
		Only one instance of it should be present, so it uses a singleton pattern.
		Will automatically destroy all systems when it's out of scope.

		Contains the ONLY instances of game systems in the game. No one else is allowed to create the game systems.
		As such, all code are to get system pointers only from the CoreEngine.
	*/
	class CoreEngine
	{
	public:
		//A class type used to hold the order the system should be init/updated/destroyed, and other data necessary.
		struct CoreSystem
		{
			CoreSystem(std::string name, std::unique_ptr<ISystem> system, char init_order, char update_order, char destruct_order)
				: name{name}, system{std::move(system)}, init_order{init_order}, update_order{update_order}, destruct_order{destruct_order}
			{}
			//Used to identify the system, like for debug purposes.
			std::string name;
			//Use a pointer to prevent slicing, smart pointer for RAII destruction call, unique_ptr for sole ownership.
			std::unique_ptr<ISystem> system;
			//Specified orders of initialization, Update, Destruction. These orders aren't necessarily the same.
			char init_order{}, update_order{}, destruct_order{};

			// FOR PROFILER 
			std::string label_update; // Precomputed sys::<name>::update
			std::string label_post; // Precomputed sys::<name>::postupdate
		};

		//Singleton pattern: Everyone shares the only instance of the core engine.
		static CoreEngine& GetInstance();

		/*
			Add a system under CoreEngine management.
			Systems do not need to be added in any particular order, but the order of init/update/destruction needs to be specified (relative to other systems added).
			Add all systems before calling initialize.
		*/
		void AddSystem(std::string name, std::unique_ptr<ISystem> system, char init_order, char update_order, char destruct_order);
		
		/*
			Initializes all systems in order.
			Call after adding all systems, before update.
		*/
		void InitializeSystems();

		/*
			Updates all systems in order.
			Call every frame.
		*/
		void UpdateSystems(float delta_time);

		/*
			PostUpdates all systems in order.
			Call every frame, after update.
		*/
		void PostUpdateSystems(float delta_time);

		//Rule of 0, delete as they shouldn't be used.
		CoreEngine(const CoreEngine& rhs) = delete;
		CoreEngine& operator=(const CoreEngine& rhs) = delete;

	private:
		//Keep it private, as a singleton it should not have multiple instances. Call GetInstance instead.
		CoreEngine() = default;

		//Private destructor. Automatically destroys all systems still in its control.
		~CoreEngine();

		//Array containing all systems. Sorted based on order of init/update/destruct according to current needs.
		std::vector<CoreSystem> system_arr;

		// Helper functions to safely call system methods with try-catch and error logging.
		void SafeUpdateSystem(CoreSystem& system, float delta_time);

		void SafePostUpdateSystem(CoreSystem& system, float delta_time);
	};
}

#endif