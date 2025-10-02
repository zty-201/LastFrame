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
	

	
	//Templated class, to provide different function overloads for each type that all do the same thing.
	template <typename T>
	class Emplacer
	{
	public:
		//Taking in entt::meta_any*, as references do not work.
		static bool EmplaceComponent(entt::entity entity, const entt::meta_any* data)
		{
			//Make a copy using the ptr provided
			registry.emplace<T>(entity, data->cast<T>());
		}
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

			Register all types here, like glm::vec3.
			Ensure all types to be reflected are added, so serialization/deserialization can compare against the type using entt::resolve<new type>().

			Use the name to identify the type.
			To be compatible with serialization, DO NOT use hashed strings. Simply put the name and it'll auto set the hash_string + name.

			entt::meta_factory<my_type>{}
				.type("name")   //name to identify this type
				.data<&my_type::static_variable>("static")  //name to identify that member.
				.data<&my_type::data_member>("member")	//data members, static data members and global variables can be added.
				.data<&global_variable>("global")
				.func<&my_type::static_function>("static")  //func members, static func members and global funcs can be added.
				.func<&my_type::member_function>("member")
				.func<&free_function>("free");

			If a variable/function is registered under my_type, they'll be treated as members of my_type (even for global variables/functions).
		
			Every component needs an emplace function for that type.
			.func<&Emplacer<My_Component>::EmplaceComponent>("emplace"_hs);

			NOTE: Taking in references may not work for functions registered.
		*/

		entt::meta_factory<EngineFramework::TODELETE>()
			.type("TestClass")
			.data<&TODELETE::a>("player_health")
			.data<&TODELETE::b>("b")
			.data<&TODELETE::c>("c")
			.func<&Emplacer<TODELETE>::EmplaceComponent>("emplace");

		entt::meta_factory<EngineFramework::TODELETE2>()
			.type("TestClass2")
			.data<&TODELETE2::d>("player_damage")
			.data<&TODELETE2::e>("e")
			.data<&TODELETE2::c>("c")
			.func<&Emplacer<TODELETE2>::EmplaceComponent>("emplace");

	}


	//Helper function to cast json to the correct data type, and set comp_data.
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data)
	{
		if (!data_type) return; //Not a valid data member, it could be a function etc.
		entt::meta_type type = data_type.type();
		if (type.is_integral())
		{
			if (type.is_signed())
			{
				if (type == entt::resolve<int>()) {
					data_type.set(comp_data, data.get<int>());
					return;
				}
				else if (type == entt::resolve<signed char>()) {
					data_type.set(comp_data, data.get<signed char>());
					return;
				}
				else if (type == entt::resolve<short>()) {
					data_type.set(comp_data, data.get<short>());
					return;
				}
				else if (type == entt::resolve<long>()) {
					data_type.set(comp_data, data.get<long>());
					return;
				}
				else if (type == entt::resolve<long long>()) {
					data_type.set(comp_data, data.get<long long>());
					return;
				}
			}
			else
			{
				if (type == entt::resolve<bool>()) {
					data_type.set(comp_data, data.get<bool>());
					return;
				}
				else if (type == entt::resolve<unsigned char>()) {
					data_type.set(comp_data, data.get<unsigned char>());
					return;
				}
				else if (type == entt::resolve<unsigned short>()) {
					data_type.set(comp_data, data.get<unsigned short>());
					return;
				}

				else if (type == entt::resolve<unsigned int>()) {
					data_type.set(comp_data, data.get<unsigned int>());
					return;
				}
				else if (type == entt::resolve<unsigned long>()) {
					data_type.set(comp_data, data.get<unsigned long>());
					return;
				}
				else if (type == entt::resolve<unsigned long long>()) {
					data_type.set(comp_data, data.get<unsigned long long>());
					return;
				}
			}
			if (type == entt::resolve<char>()) {
				data_type.set(comp_data, data.get<char>());
				return;
			}
			else if (type == entt::resolve<wchar_t>()) {
				data_type.set(comp_data, data.get<wchar_t>());
				return;
			}
		}
		else
		{
			if (type == entt::resolve<float>()) {
				data_type.set(comp_data, data.get<float>());
				return;
			}
			else if (type == entt::resolve<std::string>()) {
				data_type.set(comp_data, data.get<std::string>());
				return;
			}
			else if (type == entt::resolve<double>()) {
				data_type.set(comp_data, data.get<double>());
				return;
			}
			else if (type == entt::resolve<long double>()) {
				data_type.set(comp_data, data.get<long double>());
				return;
			}
			else if (type == entt::resolve<std::nullptr_t>()) {
				data_type.set(comp_data, data.get<nullptr_t>());
				return;
			}
		}
		//Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
		if (type == entt::resolve<char8_t>()) {
			data_type.set(comp_data, data.get<char8_t>());
			return;
		}
		else if (type == entt::resolve<char16_t>()) {
			data_type.set(comp_data, data.get<char16_t>());
			return;
		}
		else if (type == entt::resolve<char32_t>()) {
			data_type.set(comp_data, data.get<char32_t>());
			return;
		}
		//not supported by nlohmann json library.
		/*else if (type == entt::resolve<void*>()) {
			callable(data.get({}).cast<void*>());
		}*/

		assert(false && "Type is not supported by SetMetaAnyWithJson function. Please add it here.");

	}



	//Helper function to cast meta_any member_data to the correct json data type, and set comp_json.
	void SetJsonWithMetaAny(nlohmann::json& comp_json, const entt::meta_data& data_type, const entt::meta_any& data)
	{
		if (!data_type) return; //Not a valid data member, it could be a function etc.
		//Get the string to set the json.
		std::string name = data_type.name();
		if (name.empty())
		{
			throw std::exception("Meta Data does not have a name, was likely registered with a _hs hashed string instead of a string.");
		}
		//Compare types
		entt::meta_type type = data_type.type();
		
		if (type.is_integral())
		{
			if (type.is_signed())
			{
				if (type == entt::resolve<int>()) {
					comp_json[name] = data.cast<int>();
					return;
				}
				else if (type == entt::resolve<signed char>()) {
					comp_json[name] = data.cast<signed char>();
					return;
				}
				else if (type == entt::resolve<short>()) {
					comp_json[name] = data.cast<short>();
					return;
				}
				else if (type == entt::resolve<long>()) {
					comp_json[name] = data.cast<long>();
					return;
				}
				else if (type == entt::resolve<long long>()) {
					comp_json[name] = data.cast<long long>();
					return;
				}
			}
			else
			{
				if (type == entt::resolve<bool>()) {
					comp_json[name] = data.cast<bool>();
					return;
				}
				else if (type == entt::resolve<unsigned char>()) {
					comp_json[name] = data.cast<unsigned char>();
					return;
				}
				else if (type == entt::resolve<unsigned short>()) {
					comp_json[name] = data.cast<unsigned short>();
					return;
				}

				else if (type == entt::resolve<unsigned int>()) {
					comp_json[name] = data.cast<unsigned int>();
					return;
				}
				else if (type == entt::resolve<unsigned long>()) {
					comp_json[name] = data.cast<unsigned long>();
					return;
				}
				else if (type == entt::resolve<unsigned long long>()) {
					comp_json[name] = data.cast<unsigned long long>();
					return;
				}
			}
			if (type == entt::resolve<char>()) {
				comp_json[name] = data.cast<char>();
				return;
			}
			else if (type == entt::resolve<wchar_t>()) {
				comp_json[name] = data.cast<wchar_t>();
				return;
			}
		}
		else
		{
			if (type == entt::resolve<float>()) {
				comp_json[name] = data.cast<float>();
				return;
			}
			else if (type == entt::resolve<std::string>()) {
				comp_json[name] = data.cast<std::string>();
				return;
			}
			else if (type == entt::resolve<double>()) {
				comp_json[name] = data.cast<double>();
				return;
			}
			else if (type == entt::resolve<long double>()) {
				comp_json[name] = data.cast<long double>();
				return;
			}
			else if (type == entt::resolve<std::nullptr_t>()) {
				comp_json[name] = data.cast<nullptr_t>();
				return;
			}
		}
		//Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
		if (type == entt::resolve<char8_t>()) {
			comp_json[name] = data.cast<char8_t>();
			return;
		}
		else if (type == entt::resolve<char16_t>()) {
			comp_json[name] = data.cast<char16_t>();
			return;
		}
		else if (type == entt::resolve<char32_t>()) {
			comp_json[name] = data.cast<char32_t>();
			return;
		}
		//not supported by nlohmann json library.
		/*else if (type == entt::resolve<void*>()) {
			callable(data.get({}).cast<void*>());
		}*/

		assert(false && "Type is not supported by SetMetaAnyWithJson function. Please add it here.");

	}
}