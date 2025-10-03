/*
	Initializes reflection system with each component's reflection data.
	Reflection registration is centralized in this header and cpp file so most reflection logic is kept separate from the rest.
*/

#include "pch.hpp"
#include "ReflectionRegister.hpp"
//==Components for registering==//
#include "System_GameObjectSystem.hpp" //TODO: delete after testing.

/*
	This is for third-party types.
	For your own types, simply put to/from_json in global namespace or in the respective namespace of the type e.g. namespace hello{ to/fromjson}
	If not, you can use the same template too.
*/
NLOHMANN_JSON_NAMESPACE_BEGIN
template<>
struct adl_serializer<glm::vec3> {
	static void to_json(nlohmann::json& j, const glm::vec3& p)
	{
		j = nlohmann::json{ p.x, p.y, p.z };
	}
	static void from_json(const nlohmann::json& j, glm::vec3& p)
	{
		p.x = j[0];
		p.y = j[1];
		p.z = j[2];
	}
};

template<>
struct adl_serializer<glm::mat4> {
	static void to_json(json& j, const glm::mat4& m) {

		j = json::array({
			{ m[0][0], m[0][1], m[0][2], m[0][3] },
			{ m[1][0], m[1][1], m[1][2], m[1][3] },
			{ m[2][0], m[2][1], m[2][2], m[2][3] },
			{ m[3][0], m[3][1], m[3][2], m[3][3] }
			});
	}

	static void from_json(const json& j, glm::mat4& m) {
		if (!j.is_array() || j.size() != 4)
			throw std::exception("Array must be size 4x4 for mat4 serialization from json");

		for (int i = 0; i < 4; ++i) {
			if (!j[i].is_array() || j[i].size() != 4)
				throw std::exception("Array must be size 4x4 for mat4 serialization from json");

			for (int k = 0; k < 4; ++k) {
				m[i][k] = j[i][k].get<float>();
			}
		}
	}
};
NLOHMANN_JSON_NAMESPACE_END

using namespace entt::literals;
namespace EngineFramework
{

	//For use with SetMetaAny and SetJson functions. 
	template <typename T>
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data)
	{
		data_type.set(comp_data, data.get<T>());
	}
	template <typename T>
	void SetJsonWithMetaAny(nlohmann::json& comp_json, const std::string& name, const entt::meta_any& data)
	{
		comp_json[name] = data.cast<T>();
	}

	/*
		Note: When modifying maps to include arbitrary types, ensure to_json and from_json methods are declared for your type at the top of this file.
	*/

	//Basically puts partial specialisations of the template function into a map together with the associated type id.
#define SetMeta(T) {entt::resolve<T>().id(), SetMetaAnyWithJson<T>}
	static std::unordered_map<entt::id_type, std::function<void(entt::meta_any&, const entt::meta_data&, const nlohmann::json&)>> set_meta_with_json_map
	{
		SetMeta(signed char),
		SetMeta(short),
		SetMeta(int),
		SetMeta(long),
		SetMeta(long long),
		SetMeta(bool),
		SetMeta(unsigned char),
		SetMeta(unsigned short),
		SetMeta(unsigned int),
		SetMeta(unsigned long),
		SetMeta(unsigned long long),
		SetMeta(char),
		SetMeta(wchar_t),
		SetMeta(float),
		SetMeta(double),
		SetMeta(long double),
		SetMeta(std::string),
		SetMeta(std::nullptr_t),
		SetMeta(char8_t),
		SetMeta(char16_t),
		SetMeta(char32_t),
		SetMeta(glm::vec3),
		SetMeta(glm::mat4),
		SetMeta(std::vector<int>),
		SetMeta(std::vector<float>),
		SetMeta(std::vector<std::string>)
	};
#define SetJson(T) {entt::resolve<T>().id(), SetJsonWithMetaAny<T>}
	static std::unordered_map<entt::id_type, std::function<void(nlohmann::json&, const std::string&, const entt::meta_any&)>> set_json_with_meta_map
	{
		SetJson(signed char),
		SetJson(short),
		SetJson(int),
		SetJson(long),
		SetJson(long long),
		SetJson(bool),
		SetJson(unsigned char),
		SetJson(unsigned short),
		SetJson(unsigned int),
		SetJson(unsigned long),
		SetJson(unsigned long long),
		SetJson(char),
		SetJson(wchar_t),
		SetJson(float),
		SetJson(double),
		SetJson(long double),
		SetJson(std::string),
		SetJson(std::nullptr_t),
		SetJson(char8_t),
		SetJson(char16_t),
		SetJson(char32_t),
		SetJson(glm::vec3),
		SetJson(glm::mat4),
		SetJson(std::vector<int>),
		SetJson(std::vector<float>),
		SetJson(std::vector<std::string>)
	};



	//Templated class, to provide different function overloads for each type that all do the same thing.
	template <typename T>
	class Emplacer
	{
	public:
		//Taking in entt::meta_any*, as references do not work.
		static bool EmplaceComponent(entt::entity entity, const void* data)
		{
			//Don't emplace duplicate components, so check if it exists in entity first.
			if (registry.all_of<T>(entity))
			{
				return false;
			}
			//Make a copy using the ptr provided
			registry.emplace<T>(entity, static_cast<const entt::meta_any*>(data)->cast<T>());
			return true;
		}
	};

	//Templated class, to provide different function overloads for each type that all do the same thing.
	template <typename T>
	class Getter
	{
	public:
		//Taking in entt::meta_any*, as references do not work.
		static T GetComponent(entt::entity entity)
		{
			return registry.get<T>(entity);
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
			NOTE: Taking in references may not work for functions registered. If it doesn't, use pointers.


		1. Every component needs an emplace and getter function for that type.
			.func<&Emplacer<My_Component>::EmplaceComponent>("emplace"_hs)
			.func<&Getter<TODELETE>::GetComponent>("get");

		2. Every arbitrary (not c++ standard) type that isn't a component needs to be registered too.
		//Register the non-component arbitrary types used.
		entt::meta_factory<glm::vec3>()
			.traits(REGISTERED_TYPE::NON_COMPONENT);
		//Register the non-component arbitrary types used.
		entt::meta_factory<glm::mat4>()
			.traits(REGISTERED_TYPE::NON_COMPONENT);

			IF YOUR COMPONENT USES AN ARBITRARY TYPE THAT ISN'T REGISTERED, YOU"LL GET AN ERROR.

		3. After registering an arbitrary type, define to_json and from_json methods at the top of this file.
		   Modify set_meta_with_json and set_json_with_meta maps to include your type (simply place it at the bottom).
		*/
#define REGISTER_ARBITRARY_TYPE(T) entt::meta_factory<T>().traits(REGISTERED_TYPE::NON_COMPONENT)
		//Register the non-component arbitrary types used.
		REGISTER_ARBITRARY_TYPE(glm::vec3);
		REGISTER_ARBITRARY_TYPE(glm::mat4);


		entt::meta_factory<EngineFramework::TODELETE>()
			.type("TestClass")
			.data<&TODELETE::a>("player_health")
			.data<&TODELETE::b>("b")
			.data<&TODELETE::c>("c")
			.func<&Emplacer<TODELETE>::EmplaceComponent>("emplace")
			.func<&Getter<TODELETE>::GetComponent>("get");

		//If necessary, may want to add a set method too.
		entt::meta_factory<EngineFramework::TODELETE2>()
			.type("TestClass2")
			.data<&TODELETE2::d>("player_damage")
			.data<&TODELETE2::e>("e")
			.data<&TODELETE2::c>("c")
			.func<&Emplacer<TODELETE2>::EmplaceComponent>("emplace")
			.func<&Getter<TODELETE2>::GetComponent>("get");

		entt::meta_factory<EngineFramework::TODELETE3>()
			.type("TestClass3")
			.data<&TODELETE3::pos>("player_pos")
			.func<&Emplacer<TODELETE3>::EmplaceComponent>("emplace")
			.func<&Getter<TODELETE3>::GetComponent>("get");

	}


	//Helper function to cast json to the correct data type, and set comp_data.
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data)
	{
		if (!data_type) return; //Not a valid data member, it could be a function etc.
		entt::id_type type = data_type.type().id();

		//Call the respective template function if successful matching.
		if (set_meta_with_json_map.find(type) != set_meta_with_json_map.end())
		{
			set_meta_with_json_map[type](comp_data, data_type, data);
			return;
		}
		//Else throw an assertion.
		assert(false && "Type is not supported by SetMetaAnyWithJson function. Please add to map.");
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
		entt::id_type type = data_type.type().id();

		//Call the respective template function if successful matching.
		if (set_json_with_meta_map.find(type) != set_json_with_meta_map.end())
		{
			set_json_with_meta_map[type](comp_json, name, data);
			return;
		}
		//Else throw an assertion.
		assert(false && "Type is not supported by SetJsonWithMetaAny function. Please add to map.");

	}
}

//if (type.is_integral())
		//{
		//	if (type.is_signed())
		//	{
		//		if (type == entt::resolve<int>()) {
		//			data_type.set(comp_data, data.get<int>());
		//			return;
		//		}
		//		else if (type == entt::resolve<signed char>()) {
		//			data_type.set(comp_data, data.get<signed char>());
		//			return;
		//		}
		//		else if (type == entt::resolve<short>()) {
		//			data_type.set(comp_data, data.get<short>());
		//			return;
		//		}
		//		else if (type == entt::resolve<long>()) {
		//			data_type.set(comp_data, data.get<long>());
		//			return;
		//		}
		//		else if (type == entt::resolve<long long>()) {
		//			data_type.set(comp_data, data.get<long long>());
		//			return;
		//		}
		//	}
		//	else
		//	{
		//		if (type == entt::resolve<bool>()) {
		//			data_type.set(comp_data, data.get<bool>());
		//			return;
		//		}
		//		else if (type == entt::resolve<unsigned char>()) {
		//			data_type.set(comp_data, data.get<unsigned char>());
		//			return;
		//		}
		//		else if (type == entt::resolve<unsigned short>()) {
		//			data_type.set(comp_data, data.get<unsigned short>());
		//			return;
		//		}

		//		else if (type == entt::resolve<unsigned int>()) {
		//			data_type.set(comp_data, data.get<unsigned int>());
		//			return;
		//		}
		//		else if (type == entt::resolve<unsigned long>()) {
		//			data_type.set(comp_data, data.get<unsigned long>());
		//			return;
		//		}
		//		else if (type == entt::resolve<unsigned long long>()) {
		//			data_type.set(comp_data, data.get<unsigned long long>());
		//			return;
		//		}
		//	}
		//	//May not be treated as signed or unsigned, so place outside.

		//	if (type == entt::resolve<char>()) {
		//		data_type.set(comp_data, data.get<char>());
		//		return;
		//	}
		//	else if (type == entt::resolve<wchar_t>()) {
		//		data_type.set(comp_data, data.get<wchar_t>());
		//		return;
		//	}
		//}
		////Non integral types.
		//else
		//{
		//	if (type == entt::resolve<float>()) {
		//		data_type.set(comp_data, data.get<float>());
		//		return;
		//	}
		//	else if (type == entt::resolve<std::string>()) {
		//		data_type.set(comp_data, data.get<std::string>());
		//		return;
		//	}
		//	else if (type == entt::resolve<double>()) {
		//		data_type.set(comp_data, data.get<double>());
		//		return;
		//	}
		//	else if (type == entt::resolve<long double>()) {
		//		data_type.set(comp_data, data.get<long double>());
		//		return;
		//	}
		//	else if (type == entt::resolve<std::nullptr_t>()) {
		//		data_type.set(comp_data, data.get<nullptr_t>());
		//		return;
		//	}
		//}
		////Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
		//if (type == entt::resolve<char8_t>()) {
		//	data_type.set(comp_data, data.get<char8_t>());
		//	return;
		//}
		//else if (type == entt::resolve<char16_t>()) {
		//	data_type.set(comp_data, data.get<char16_t>());
		//	return;
		//}
		//else if (type == entt::resolve<char32_t>()) {
		//	data_type.set(comp_data, data.get<char32_t>());
		//	return;
		//}

		//not supported by nlohmann json library: void*, void.
//
//
//if (type.is_integral())
//{
//	if (type.is_signed())
//	{
//		if (type == entt::resolve<int>()) {
//			comp_json[name] = data.cast<int>();
//			return;
//		}
//		else if (type == entt::resolve<signed char>()) {
//			comp_json[name] = data.cast<signed char>();
//			return;
//		}
//		else if (type == entt::resolve<short>()) {
//			comp_json[name] = data.cast<short>();
//			return;
//		}
//		else if (type == entt::resolve<long>()) {
//			comp_json[name] = data.cast<long>();
//			return;
//		}
//		else if (type == entt::resolve<long long>()) {
//			comp_json[name] = data.cast<long long>();
//			return;
//		}
//	}
//	else
//	{
//		if (type == entt::resolve<bool>()) {
//			comp_json[name] = data.cast<bool>();
//			return;
//		}
//		else if (type == entt::resolve<unsigned char>()) {
//			comp_json[name] = data.cast<unsigned char>();
//			return;
//		}
//		else if (type == entt::resolve<unsigned short>()) {
//			comp_json[name] = data.cast<unsigned short>();
//			return;
//		}
//
//		else if (type == entt::resolve<unsigned int>()) {
//			comp_json[name] = data.cast<unsigned int>();
//			return;
//		}
//		else if (type == entt::resolve<unsigned long>()) {
//			comp_json[name] = data.cast<unsigned long>();
//			return;
//		}
//		else if (type == entt::resolve<unsigned long long>()) {
//			comp_json[name] = data.cast<unsigned long long>();
//			return;
//		}
//	}
//	//May not be treated as signed or unsigned, so place outside.
//	if (type == entt::resolve<char>()) {
//		comp_json[name] = data.cast<char>();
//		return;
//	}
//	else if (type == entt::resolve<wchar_t>()) {
//		comp_json[name] = data.cast<wchar_t>();
//		return;
//	}
//}
////Non-integral types
//else
//{
//	if (type == entt::resolve<float>()) {
//		comp_json[name] = data.cast<float>();
//		return;
//	}
//	else if (type == entt::resolve<std::string>()) {
//		comp_json[name] = data.cast<std::string>();
//		return;
//	}
//	else if (type == entt::resolve<double>()) {
//		comp_json[name] = data.cast<double>();
//		return;
//	}
//	else if (type == entt::resolve<long double>()) {
//		comp_json[name] = data.cast<long double>();
//		return;
//	}
//	else if (type == entt::resolve<std::nullptr_t>()) {
//		comp_json[name] = data.cast<nullptr_t>();
//		return;
//	}
//}
////Not treated as integrals before c++ 20, but is afterwards. Put outside both checks.
//if (type == entt::resolve<char8_t>()) {
//	comp_json[name] = data.cast<char8_t>();
//	return;
//}
//else if (type == entt::resolve<char16_t>()) {
//	comp_json[name] = data.cast<char16_t>();
//	return;
//}
//else if (type == entt::resolve<char32_t>()) {
//	comp_json[name] = data.cast<char32_t>();
//	return;
//}
////not supported by nlohmann json library: void*, void.