/*
	Initializes reflection system with each component's reflection data.
	Reflection registration is centralized in this header and cpp file so most reflection logic is kept separate from the rest.
*/


/*
	Usage of reflection system.
	When registering struct/class type, a name can be assigned to it.
	When registering members like variables and functions under a specific type, a name can be assigned to it.


	// direct access to a reflected type
	auto by_type = entt::resolve<my_type>();
	// look up a reflected type by identifier
	auto by_id = entt::resolve("reflected_type"_hs);
	// look up a reflected type by type info
	auto by_type_id = entt::resolve(entt::type_id<my_type>());

	//Iterating over all class/struct types.
	for(auto &&[id, type]: entt::resolve()) {
	// ...
	}

	//Getting members of a type.
	auto data = entt::resolve<my_type>().data("member"_hs);
	auto func = entt::resolve<my_type>().func("member"_hs);

	//Iterating over all members of a type
	for(auto &&[id, type]: entt::resolve<my_type>().base()) {
	// ...

	entt::resolve will return an entt::meta_type.
	entt::meta_type type = entt::resolve...
	type.construct(); //can be used to construct new data of that type.
	type.data("name").set() and type.data("name").get(); //can be used to get/set values for data members of that type.
	if(type) ... //can be used to check if the type is valid. meta_type implicitly converts to boolean.
}

*/

#ifndef REFLECTION_REGISTER_HPP
#define REFLECTION_REGISTER_HPP
#include "entt/meta/meta.hpp"
#include "entt/meta/resolve.hpp"
#include <variant>
namespace EngineFramework
{

	/*
		Called during program startup: Will register all components into Entt reflection system.
	*/
	void RegisterReflection();

	//Helper function to cast any meta type.
	template <typename Func>
	void CastMetaDataToType(entt::meta_data data, Func callable)
	{
		entt::meta_type type = data.type();

		if (type.is_integral())
		{
			if (type.is_signed())
			{
				if (type == entt::resolve<int>()) {
					callable(data.get({}).cast<int>());
					return;
				}
				else if (type == entt::resolve<signed char>()) {
					callable(data.get({}).cast<signed char>());
					return;
				}
				else if (type == entt::resolve<short>()) {
					callable(data.get({}).cast<short>());
					return;
				}
				else if (type == entt::resolve<long>()) {
					callable(data.get({}).cast<long>());
					return;
				}
				else if (type == entt::resolve<long long>()) {
					callable(data.get({}).cast<long long>());
					return;
				}
			}
			else
			{
				if (type == entt::resolve<bool>()) {
					callable(data.get({}).cast<bool>());
					return;
				}
				else if (type == entt::resolve<unsigned char>()) {
					callable(data.get({}).cast<unsigned char>());
					return;
				}
				else if (type == entt::resolve<unsigned short>()) {
					callable(data.get({}).cast<unsigned short>());
					return;
				}

				else if (type == entt::resolve<unsigned int>()) {
					callable(data.get({}).cast<unsigned int>());
					return;
				}
				else if (type == entt::resolve<unsigned long>()) {
					callable(data.get({}).cast<unsigned long>());
					return;
				}
				else if (type == entt::resolve<unsigned long long>()) {
					callable(data.get({}).cast<unsigned long long>());
					return;
				}
			}
			if (type == entt::resolve<char>()) {
				callable(data.get({}).cast<char>());
				return;
			}
			else if (type == entt::resolve<wchar_t>()) {
				callable(data.get({}).cast<wchar_t>());
				return;
			}
		}
		else
		{
			if (type == entt::resolve<float>()) {
				callable(data.get({}).cast<float>());
				return;
			}
			else if (type == entt::resolve<double>()) {
				callable(data.get({}).cast<double>());
				return;
			}
			else if (type == entt::resolve<long double>()) {
				callable(data.get({}).cast<long double>());
				return;
			}
			else if (type == entt::resolve<std::nullptr_t>()) {
				callable(data.get({}).cast<std::nullptr_t>());
				return;
			}
		}
		//Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
		if (type == entt::resolve<char8_t>()) {
			callable(data.get({}).cast<char8_t>());
			return;
		}
		else if (type == entt::resolve<char16_t>()) {
			callable(data.get({}).cast<char16_t>());
			return;
		}
		else if (type == entt::resolve<char32_t>()) {
			callable(data.get({}).cast<char32_t>());
			return;
		}
		//not supported by nlohmann json library.
		/*else if (type == entt::resolve<void*>()) {
			callable(data.get({}).cast<void*>());
		}*/

		assert(false && "Type is not supported by this function. Please add it here.");

	}
}


#endif