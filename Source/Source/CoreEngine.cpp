/*
	Implementation of CoreEngine, the backbone class that manages all game systems.
	//TODO: Unit tests
*/

#include "pch.hpp"
#include <algorithm>
#include "CoreEngine.hpp"
namespace EngineFramework
{
	//Singleton pattern: Everyone shares the only instance of the core engine.
	CoreEngine& CoreEngine::GetInstance()
	{
		static CoreEngine engine;
		return engine;
	}

	/*
		Add a system under CoreEngine management.
		Systems do not need to be added in any particular order, but the order of init/update/destruction needs to be specified (relative to other systems added).
		Add all systems before calling initialize.
	*/
	void CoreEngine::AddSystem(std::string name, std::unique_ptr<ISystem> system, char init_order, char update_order, char destruct_order)
	{
		system_arr.emplace_back(name, std::move(system), init_order, update_order, destruct_order);
	}

	/*
		Initializes all systems in order.
		Call after adding all systems, before update.
	*/
	void CoreEngine::InitializeSystems()
	{
		//Sort from smallest to largest, so that the first system to be initialized is at the front of the array.
		std::sort(system_arr.begin(), system_arr.end(), [](CoreSystem& first, CoreSystem& second) {
			return first.init_order < second.init_order;
			});

		for (CoreSystem& system : system_arr)
		{
			system.system->Initialize();
		}

		//Sort from smallest to largest, so that the first system to be updated is at the front of the array.
		//Do this in init to prevent having to sort every update loop.
		std::sort(system_arr.begin(), system_arr.end(), [](CoreSystem& first, CoreSystem& second) {
			return first.update_order < second.update_order;
			});
	}

	/*
		Updates all systems in order.
		Call every frame.
	*/
	void CoreEngine::UpdateSystems(float delta_time)
	{
		//Array has already been sorted in update order, in initialize.
		for (CoreSystem& system : system_arr)
		{
			system.system->Update(delta_time);
		}
	}

	/*
		PostUpdates all systems in order.
		Call every frame, after update.
	*/
	void CoreEngine::PostUpdateSystems(float delta_time)
	{
		//Array has already been sorted in update order, in initialize.
		for (CoreSystem& system : system_arr)
		{
			system.system->PostUpdate(delta_time);
		}
	}

	//Private destructor. Automatically destroys all systems still in its control.
	CoreEngine::~CoreEngine()
	{
		//Sort from largest to smallest, so that the first system to be destroyed is at the back of the array.
		std::sort(system_arr.begin(), system_arr.end(), [](CoreSystem& first, CoreSystem& second) {
			return first.destruct_order > second.destruct_order;
			});

		//Destroy each system starting from back to front, unique_ptr will handle RAII.
		while (!system_arr.empty())
		{
			system_arr.pop_back();
		}
	}
}