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
#include <entt/entity/registry.hpp>
//==Components for registering==//
#include "System_GameObjectSystem.hpp" //TODO: delete after testing.

using namespace entt::literals;
namespace EngineFramework
{
	//TODO: Test struct, to delete.
	struct TODELETE
	{
		int a{};
		float b{};
		std::string c{};
		float not_serializable{};
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
		
			Every type needs an emplace function for that type.
			.func< [](entt::entity entity, entt::meta_any& data) {
			registry.emplace< EngineFramework::TODELETE>(entity, data.cast< EngineFramework::TODELETE>());
			} >("emplace"_hs)
		*/

		entt::meta_factory<EngineFramework::TODELETE>()
			.type("TestClass"_hs)
			.data<&TODELETE::a>("player_health"_hs)
			.data<&TODELETE::b>("b"_hs)
			.data<&TODELETE::c>("c"_hs)
			.func< [](entt::entity entity, entt::meta_any& data) {
			LOGD("%s", "TESTER");
			registry.emplace< EngineFramework::TODELETE>(entity, data.cast< EngineFramework::TODELETE>());
			} >("emplace"_hs);

		
	}


	//Helper function to cast json to the correct data type, and set comp_data.
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, nlohmann::json& data)
	{
		if (!data_type) return; //Not a valid data member, it could be a function etc.
		entt::id_type id = data_type.type().id();
		entt::meta_type type = data_type.type();
		if (type.is_integral())
		{
			if (type.is_signed())
			{
				if (type == entt::resolve<int>()) {
					comp_data.set(id, data.get<int>());
					return;
				}
				else if (type == entt::resolve<signed char>()) {
					comp_data.set(id, data.get<signed char>());
					return;
				}
				else if (type == entt::resolve<short>()) {
					comp_data.set(id, data.get<short>());
					return;
				}
				else if (type == entt::resolve<long>()) {
					comp_data.set(id, data.get<long>());
					return;
				}
				else if (type == entt::resolve<long long>()) {
					comp_data.set(id, data.get<long long>());
					return;
				}
			}
			else
			{
				if (type == entt::resolve<bool>()) {
					comp_data.set(id, data.get<bool>());
					return;
				}
				else if (type == entt::resolve<unsigned char>()) {
					comp_data.set(id, data.get<unsigned char>());
					return;
				}
				else if (type == entt::resolve<unsigned short>()) {
					comp_data.set(id, data.get<unsigned short>());
					return;
				}

				else if (type == entt::resolve<unsigned int>()) {
					comp_data.set(id, data.get<unsigned int>());
					return;
				}
				else if (type == entt::resolve<unsigned long>()) {
					comp_data.set(id, data.get<unsigned long>());
					return;
				}
				else if (type == entt::resolve<unsigned long long>()) {
					comp_data.set(id, data.get<unsigned long long>());
					return;
				}
			}
			if (type == entt::resolve<char>()) {
				comp_data.set(id, data.get<char>());
				return;
			}
			else if (type == entt::resolve<wchar_t>()) {
				comp_data.set(id, data.get<wchar_t>());
				return;
			}
		}
		else
		{
			if (type == entt::resolve<float>()) {
				comp_data.set(id, data.get<float>());
				return;
			}
			else if (type == entt::resolve<std::string>()) {
				comp_data.set(id, data.get<std::string>());
				return;
			}
			else if (type == entt::resolve<double>()) {
				comp_data.set(id, data.get<double>());
				return;
			}
			else if (type == entt::resolve<long double>()) {
				comp_data.set(id, data.get<long double>());
				return;
			}
			else if (type == entt::resolve<std::nullptr_t>()) {
				comp_data.set(id, data.get<nullptr_t>());
				return;
			}
		}
		//Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
		if (type == entt::resolve<char8_t>()) {
			comp_data.set(id, data.get<char8_t>());
			return;
		}
		else if (type == entt::resolve<char16_t>()) {
			comp_data.set(id, data.get<char16_t>());
			return;
		}
		else if (type == entt::resolve<char32_t>()) {
			comp_data.set(id, data.get<char32_t>());
			return;
		}
		//not supported by nlohmann json library.
		/*else if (type == entt::resolve<void*>()) {
			callable(data.get({}).cast<void*>());
		}*/

		assert(false && "Type is not supported by this function. Please add it here.");

	}
}