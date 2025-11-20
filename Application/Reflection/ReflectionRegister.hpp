/*
	Initializes reflection system with each component's reflection data.
	Reflection registration is centralized in this header and cpp file so most reflection logic is kept separate from the rest.
*/


/*
	Usage of reflection system.
	When registering struct/class type, a name can be assigned to it.
	When registering members like variables and functions under a specific type, a name can be assigned to it.


	When looking up a name, ensure it's hashed if it was registered as hashed (_hs). For non-hashed registration, it can be looked up via hash too.
	for entt::id_type, use entt::hashed_string{"string"} to get the hashed string.

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

/*
	Type resolving.
	SetJsonWithMetaAny() and SetMetaAnyWithJson() is used to convert meta_any into a concrete type, or json into a concrete type.
	They are used during serialization and deserialization respectively, so any new data types to account for needs to be placed into both.

	CastMetaDataToType is optional, but it may be used. if used, the new data types need to be placed in this function too.

	When setting new data types, sometimes json doesn't have implicit support for these types. e.g. glm::vec3, std::vector<T>.
	In these cases, instead of doing data.cast<T> the data may need to be written to json or read from json in a different way. e.g. reading/writing glm::vec3 as an array of 3 floats.
	Try to keep code reusable, e.g. std::vector<T> read/write to json can be placed into a template function so it can be reused for different types like std::vector<int>, std::vector<float>.
	Also ensure data types are registered before using entt::resolve<new type>(). e.g. register glm::vec3.
*/

#ifndef REFLECTION_REGISTER_HPP
#define REFLECTION_REGISTER_HPP
#include "entt/meta/meta.hpp"
#include "entt/meta/resolve.hpp"
#include "nlohmann/json.hpp"
#include <variant>
namespace EngineFramework
{
	inline entt::id_type GetID(std::string type_name)
	{
		return entt::hashed_string{ type_name.c_str() }.value();
	}
	/*
		Called during program startup: Will register all components into Entt reflection system.
	*/
	void RegisterReflection();

	//Helper function to cast json to the correct data type, and set comp_data.
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data);
	//Helper function to cast meta_any to correct data type, and set comp_json.
	void SetJsonWithMetaAny(nlohmann::json& comp_json, const entt::meta_data& data_type, const entt::meta_any& data);

	//Used to differentiate between the different types.
	enum struct REGISTERED_TYPE : short
	{
		COMPONENT = 0,
		NON_COMPONENT = 1
	};
	
	struct RequiresSerializedHistoryTag {};

	//Helper function to cast any meta type.
	//template <typename Func>
	//void CastMetaDataToType(const entt::meta_data &data_type, nlohmann::json& data, Func callable)
	//{
	//	entt::meta_type type = data_type.type();
	//	if (type.is_integral())
	//	{
	//		if (type.is_signed())
	//		{
	//			if (type == entt::resolve<int>()) {
	//				callable(data.get<int>());
	//				return;
	//			}
	//			else if (type == entt::resolve<signed char>()) {
	//				callable(data.get<signed char>());
	//				return;
	//			}
	//			else if (type == entt::resolve<short>()) {
	//				callable(data.get<short>());
	//				return;
	//			}
	//			else if (type == entt::resolve<long>()) {
	//				callable(data.get<long>());
	//				return;
	//			}
	//			else if (type == entt::resolve<long long>()) {
	//				callable(data.get<long long>());
	//				return;
	//			}
	//		}
	//		else
	//		{
	//			if (type == entt::resolve<bool>()) {
	//				callable(data.get<bool>());
	//				return;
	//			}
	//			else if (type == entt::resolve<unsigned char>()) {
	//				callable(data.get<unsigned char>());
	//				return;
	//			}
	//			else if (type == entt::resolve<unsigned short>()) {
	//				callable(data.get<unsigned short>());
	//				return;
	//			}

	//			else if (type == entt::resolve<unsigned int>()) {
	//				callable(data.get<unsigned int>());
	//				return;
	//			}
	//			else if (type == entt::resolve<unsigned long>()) {
	//				callable(data.get<unsigned long>());
	//				return;
	//			}
	//			else if (type == entt::resolve<unsigned long long>()) {
	//				callable(data.get<unsigned long long>());
	//				return;
	//			}
	//		}
	//		if (type == entt::resolve<char>()) {
	//			callable(data.get<char>());
	//			return;
	//		}
	//		else if (type == entt::resolve<wchar_t>()) {
	//			callable(data.get<wchar_t>());
	//			return;
	//		}
	//	}
	//	else
	//	{
	//		if (type == entt::resolve<float>()) {
	//			callable(data.get<float>());
	//			return;
	//		}
	//		else if (type == entt::resolve<std::string>()) {
	//			callable(data.get<std::string>());
	//			return;
	//		}
	//		else if (type == entt::resolve<double>()) {
	//			callable(data.get<double>());
	//			return;
	//		}
	//		else if (type == entt::resolve<long double>()) {
	//			callable(data.get<long double>());
	//			return;
	//		}
	//		else if (type == entt::resolve<std::nullptr_t>()) {
	//			callable(data.get<nullptr_t>());
	//			return;
	//		}
	//	}
	//	//Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
	//	if (type == entt::resolve<char8_t>()) {
	//		callable(data.get<char8_t>());
	//		return;
	//	}
	//	else if (type == entt::resolve<char16_t>()) {
	//		callable(data.get<char16_t>());
	//		return;
	//	}
	//	else if (type == entt::resolve<char32_t>()) {
	//		callable(data.get<char32_t>());
	//		return;
	//	}
	//	//not supported by nlohmann json library.
	//	/*else if (type == entt::resolve<void*>()) {
	//		callable(data.get({}).cast<void*>());
	//	}*/

	//	assert(false && "Type is not supported by CastMetaDataToType function. Please add it here.");

	//}

	//TODO: Test struct, to delete.
	struct TODELETE
	{
		int a{};
		float b{};
		std::string c{};
		float not_serializable{};
	};
	//TODO: Test struct, to delete.
	struct TODELETE2
	{
		int d{};
		float e{};
		std::string c{};
		float not_serializable{};
	};
	//TODO: Test struct, to delete.
	struct TODELETE3
	{
		glm::vec3 pos{};
	};
}



#endif
