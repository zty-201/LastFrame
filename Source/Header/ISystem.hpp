/*
	\brief
	ISystem interface from which all game systems derive from.
	Declares functions that derived classes have to implement, for CoreEngine to call.
*/

#ifndef I_SYSTEM_HPP
#define I_SYSTEM_HPP
namespace EngineFramework
{
	class ISystem
	{
	private:
		/*
			Used to initialize system resources before running.
			Called by CoreEngine.
			Do not initialize in constructor, as systems need to be initialized in order.
		*/
		virtual void Initialize() = 0;

		/*
			Used to update system behaviour. For example, updating all components or checking for activity.
			Called by CoreEngine.
		*/
		virtual void Update() = 0;

	public:
		/*
			Used to automatically deallocate any allocated resources after the ISystem goes out of scope/program ends.
		*/
		virtual ~ISystem() {};
	};
}
#endif