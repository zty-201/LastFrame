/*
	\brief
	ISystem interface from which all game systems derive from.
	Declares functions that derived classes have to implement, for CoreEngine to call.
*/

#ifndef I_SYSTEM_HPP
#define I_SYSTEM_HPP
#include "global.hpp"
#include "CoreEngine.hpp"
namespace EngineFramework
{
	class CoreEngine;
	/*
		The interface base class used by all derived game system classes.
		Every derived game system must implement
		- Init
		- Update
		- Destructor.

		How to implement a pseudo-singleton pattern:
		- Note that the CoreEngine is the only one who will create instances of the class, so the class must not create itself (unlike a singleton).
		- Rather, store a static pointer to itself when its ctor is run, then return the pointer when called for.
		- For example --> Graphics(){ this_graphic_system = this;}  ,  static Graphics& GetInstance(){return *this_graphic_system);
		- Alternatively, have a getter method for CoreEngine to return the ISystem pointer (and cast that to the respective class).
	*/
	class ISystem
	{
	public:
		/*
			Used to automatically deallocate any allocated resources after the ISystem goes out of scope/program ends.
		*/
		virtual ~ISystem() = default;
	private:
		//CoreEngine should be able to access this system and call the functions, but no one else.
		friend CoreEngine;
		/*
			Used to initialize system resources before running.
			Called by CoreEngine in order.
			Do not initialize in constructor, as systems need to be initialized in order.
		*/
		virtual void Initialize() = 0;

		/*
			Used to update system behaviour. For example, updating all components or checking for activity.
			Called by CoreEngine in order.
		*/
		virtual void Update(float delta_time) = 0;

		/*
			Used to perform actions after every system has been updated.
			Called by the CoreEngine in order.
			Optional.
		*/
		virtual void PostUpdate(float delta_time) { UNREFERENCED_PARAMETER(delta_time); };
	};
}
#endif