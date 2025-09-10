/*
	Initializes reflection system with each component's reflection data.
	Reflection registration is centralized in this header and cpp file so most reflection logic is kept separate from the rest.
*/

#include "pch.hpp"
#include <cassert>
#include <functional>
#include "ReflectionRegister.hpp"
//==For reflection system.
#include "entt/meta/factory.hpp" 
#include "entt/meta/meta.hpp"
#include <entt/meta/resolve.hpp>
//==Components for registering==//
#include "System_GameObjectSystem.hpp" //TODO: delete after testing.

using namespace entt::literals;
namespace EngineFramework
{
	//TODO: Test struct, to delete.
	struct TODELETE
	{
		int a;
		float b;
		std::string c;
		float not_serializable;
	};

	/*
		Called during program startup: Will register all components into Entt reflection system.
	*/
	void RegisterReflection()
	{
		/*
			Instructions:

			Register all components here.
			Only add a member to the registration if you want it to be visible during reflection. 
			--> Anything that should be saved/read from file should be registered.
			--> Anything that should not be saved/read from file should not be registered, i think.

			entt::meta_factory<my_type>{}
				.type("name"_hs)   //name to identify this type
				.data<&my_type::static_variable>("static"_hs)  //name to identify that member.
				.data<&my_type::data_member>("member"_hs)	//data members, static data members and global variables can be added.
				.data<&global_variable>("global"_hs)
				.func<&my_type::static_function>("static"_hs)  //func members, static func members and global funcs can be added.
				.func<&my_type::member_function>("member"_hs)
				.func<&free_function>("free"_hs);

			If a variable/function is registered under my_type, they'll be treated as members of my_type (even for global variables/functions).
		*/
		entt::meta_factory<EngineFramework::TODELETE>()
			.type("TestClass"_hs)
			.data<&TODELETE::a>("player_health")
			.data<&TODELETE::b>("b")
			.data<&TODELETE::c>("c");

		
	}
}