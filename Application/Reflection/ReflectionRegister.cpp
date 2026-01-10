/*
	Initializes reflection system with each component's reflection data.
	Reflection registration is centralized in this header and cpp file so most reflection logic is kept separate from the rest.
*/

#include "pch.hpp"
#include "ReflectionRegister.hpp"
//==Components for registering==//
#include "CameraComponent.h"
#include "LightComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "TransformComponent.h"
#include "AnimationComponent.h"
#include "AudioComponent.h"
#include "AudioListenerComponent.h"
#include "ColliderComponent.h"
#include "PhysicsMaterialComponent.h"
#include "PhysicsComponent.h"
#include "TriggerBoxComponent.h"
#include "UIComponent.h"
#include "Scripting/Script.hpp"
#include "System_GameObjectSystem.hpp" //TODO: delete after testing.
#include "MetaInspector.hpp"
#include "NameComponent.h"

/*
	This is for third-party types.
	For your own types, simply put to/from_json in global namespace or in the respective namespace of the type e.g. namespace hello{ to/fromjson}
	If not, you can use the same template too.
*/
NLOHMANN_JSON_NAMESPACE_BEGIN

template<>
struct adl_serializer<glm::vec2> {
	static void to_json(nlohmann::json& j, const glm::vec2& p)
	{
		j = nlohmann::json{ p.x, p.y };
	}
	static void from_json(const nlohmann::json& j, glm::vec2& p)
	{
		p.x = j[0];
		p.y = j[1];
	}
};

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
			throw std::runtime_error("Array must be size 4x4 for mat4 serialization from json");

		for (int i = 0; i < 4; ++i) {
			if (!j[i].is_array() || j[i].size() != 4)
				throw std::runtime_error("Array must be size 4x4 for mat4 serialization from json");

			for (int k = 0; k < 4; ++k) {
				m[i][k] = j[i][k].get<float>();
			}
		}
	}
};

template<>
struct adl_serializer<xresource::full_guid> {
	static void to_json(json& j, const xresource::full_guid& guid) {
		j = json::array({ guid.m_Instance.m_Value, guid.m_Type.m_Value });
	}

	static void from_json(const json& j, xresource::full_guid& guid) {
		guid.m_Instance = j[0].get<uint64_t>();
		guid.m_Type = j[1].get<uint64_t>();
	}
};

template<>
struct adl_serializer<xresource::instance_guid> {
	static void to_json(json& j, const xresource::instance_guid& guid) {
		j = guid.m_Value;
	}

	static void from_json(const json& j, xresource::instance_guid& guid) {
		guid.m_Value = j.get<uint64_t>();
	}
};

template<>
struct adl_serializer<AudioComponent::AudioClip> {
	static void to_json(json& j, const AudioComponent::AudioClip& clip) {
		j = json::object();
		j["audio_guid"] = clip.audio_guid;
		j["volume"] = clip.volume;
		j["loop"] = clip.loop;
		j["playOnStart"] = clip.playOnStart;
		j["is3D"] = clip.is3D;
		j["minDistance"] = clip.minDistance;
		j["maxDistance"] = clip.maxDistance;
	}

	static void from_json(const json& j, AudioComponent::AudioClip& clip) {
		if (j.contains("audio_guid") && !j["audio_guid"].is_null()) {
			clip.audio_guid = j["audio_guid"].get<xresource::full_guid>();
		}
		else {
			clip.audio_guid = xresource::full_guid{};
		}

		clip.volume = j.value("volume", 1.0f);
		clip.loop = j.value("loop", false);
		clip.playOnStart = j.value("playOnStart", false);
		clip.is3D = j.value("is3D", true);
		clip.minDistance = j.value("minDistance", 1.0f);
		clip.maxDistance = j.value("maxDistance", 25.0f);
		clip.playRequested = j.value("playRequested", false);
	}
};

#if defined(ENGINE_ENABLE_LUA)
template<>
struct adl_serializer<Engine::LuaScriptComponent::LuaInstance> {
	static void to_json(json& j, const Engine::LuaScriptComponent::LuaInstance& inst) {
		j = json::object();
		j["ScriptGuid"] = inst.ScriptGuid;
		j["Loaded"] = inst.Loaded;
	}

	static void from_json(const json& j, Engine::LuaScriptComponent::LuaInstance& inst) {
		if (j.contains("ScriptGuid") && !j["ScriptGuid"].is_null()) {
			inst.ScriptGuid = j["ScriptGuid"].get<xresource::instance_guid>();
		}
		else {
			inst.ScriptGuid.clear();
		}

		inst.Loaded = j.value("Loaded", false);
		inst.Env = sol::nil;
		inst.Self = sol::nil;
		inst.FnOnCreate = sol::nil;
		inst.FnOnUpdate = sol::nil;
		inst.FnOnDestroy = sol::nil;
	}
};
#endif

template<>
struct adl_serializer<CameraComponent::CameraType> {
	static void to_json(json& j, const CameraComponent::CameraType& type) {
		j = static_cast<int>(type);
	}

	static void from_json(const json& j, CameraComponent::CameraType& type) {
		type = static_cast<CameraComponent::CameraType>(j.get<int>());
	}
};

template<>
struct adl_serializer<LightComponent::Type> {
	static void to_json(json& j, const LightComponent::Type& type) {
		j = static_cast<int>(type);
	}

	static void from_json(const json& j, LightComponent::Type& type) {
		type = static_cast<LightComponent::Type>(j.get<int>());
	}
};

template<>
struct adl_serializer<TriggerShapeType> {
	static void to_json(nlohmann::json& j, const TriggerShapeType& v) {
		j = static_cast<int>(v);
	}
	static void from_json(const nlohmann::json& j, TriggerShapeType& v) {
		v = static_cast<TriggerShapeType>(j.get<int>());
	}
};

template<>
struct adl_serializer<std::filesystem::path> {
	static void to_json(json& j, const std::filesystem::path& p) {
		j = p.generic_string();
	}
	static void from_json(const json& j, std::filesystem::path& p) {
		p = j.get<std::string>();
	}
};

NLOHMANN_JSON_NAMESPACE_END

// ColliderShape Enum
inline void to_json(nlohmann::json& j, const ::ColliderShape& s) {
	j = static_cast<int>(s);
}
inline void from_json(const nlohmann::json& j, ::ColliderShape& s) {
	s = static_cast<::ColliderShape>(j.get<int>());
}

// ColliderComponent::Mode Enum (Global Scope)
inline void to_json(nlohmann::json& j, const ::ColliderComponent::Mode& m) {
	j = static_cast<int>(m);
}
inline void from_json(const nlohmann::json& j, ::ColliderComponent::Mode& m) {
	m = static_cast<::ColliderComponent::Mode>(j.get<int>());
}

inline void to_json(nlohmann::json& j, const ::ColliderData& c) {
	j = nlohmann::json{
		{"type", c.type}, // It's already an int!
		{"boxHalfExtents", c.boxHalfExtents},
		{"sphereRadius", c.sphereRadius},
		{"capsuleHeight", c.capsuleHeight},
		{"capsuleRadius", c.capsuleRadius},
		{"offsetPosition", c.offsetPosition},
		{"offsetRotation", c.offsetRotation}
	};
}
inline void from_json(const nlohmann::json& j, ::ColliderData& c) {
	if (j.contains("type")) c.type = static_cast<int>(j.at("type").get<int>()); // Direct int read
	if (j.contains("boxHalfExtents")) c.boxHalfExtents = j.at("boxHalfExtents").get<glm::vec3>();
	if (j.contains("sphereRadius")) c.sphereRadius = j.at("sphereRadius").get<float>();
	if (j.contains("capsuleHeight")) c.capsuleHeight = j.at("capsuleHeight").get<float>();
	if (j.contains("capsuleRadius")) c.capsuleRadius = j.at("capsuleRadius").get<float>();
	if (j.contains("offsetPosition")) c.offsetPosition = j.at("offsetPosition").get<glm::vec3>();
	if (j.contains("offsetRotation")) c.offsetRotation = j.at("offsetRotation").get<glm::vec3>();
}

using namespace entt::literals;
namespace EngineFramework
{
	namespace detail
	{
		template <typename T>
		void AssignMetaFromJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data)
		{
			data_type.set(comp_data, data.get<T>());
		}

		template <typename T>
		void AssignJsonFromMeta(nlohmann::json& comp_json, const std::string& name, const entt::meta_any& data)
		{
			comp_json[name] = data.cast<T>();
		}

		using MetaSetter = void(*)(entt::meta_any&, const entt::meta_data&, const nlohmann::json&);
		using JsonSetter = void(*)(nlohmann::json&, const std::string&, const entt::meta_any&);

		template <typename T>
		constexpr std::pair<entt::id_type, MetaSetter> MetaEntry()
		{
			return { entt::resolve<T>().id(), &AssignMetaFromJson<T> };
		}

		template <typename T>
		constexpr std::pair<entt::id_type, JsonSetter> JsonEntry()
		{
			return { entt::resolve<T>().id(), &AssignJsonFromMeta<T> };
		}

		const auto& GetMetaSetterMap()
		{
			using MapType = std::unordered_map<entt::id_type, MetaSetter>;
			static const MapType map = []()
			{
				MapType temp{
					MetaEntry<signed char>(),
					MetaEntry<short>(),
					MetaEntry<int>(),
					MetaEntry<long>(),
					MetaEntry<long long>(),
					MetaEntry<bool>(),
					MetaEntry<unsigned char>(),
					MetaEntry<unsigned short>(),
					MetaEntry<unsigned int>(),
					MetaEntry<unsigned long>(),
					MetaEntry<unsigned long long>(),
					MetaEntry<char>(),
					MetaEntry<wchar_t>(),
					MetaEntry<float>(),
					MetaEntry<double>(),
					MetaEntry<long double>(),
					MetaEntry<std::string>(),
					MetaEntry<std::nullptr_t>(),
					MetaEntry<char8_t>(),
					MetaEntry<char16_t>(),
					MetaEntry<char32_t>(),
					MetaEntry<glm::vec2>(),
					MetaEntry<glm::vec3>(),
					MetaEntry<glm::mat4>(),
					MetaEntry<xresource::full_guid>(),
					MetaEntry<std::vector<int>>(),
					MetaEntry<std::vector<float>>(),
					MetaEntry<std::vector<std::string>>(),
					MetaEntry<std::vector<AudioComponent::AudioClip>>(),
					MetaEntry<CameraComponent::CameraType>(),
					MetaEntry<LightComponent::Type>(),
					MetaEntry<std::filesystem::path>(),
					MetaEntry<TriggerShapeType>(),
					MetaEntry<::ColliderShape>(),
					MetaEntry<::ColliderData>(),
					MetaEntry<std::vector<::ColliderData>>(),
					MetaEntry<::ColliderComponent::Mode>(),
					MetaEntry<ColliderShape>(),
					MetaEntry<ColliderData>(),
					MetaEntry<std::vector<ColliderData>>(),
					MetaEntry<ColliderComponent::Mode>(),
					MetaEntry<xresource::instance_guid>(),
					MetaEntry<SoundBankMap>()
				};
#if defined(ENGINE_ENABLE_LUA)
				temp.insert({
					MetaEntry<Engine::LuaScriptComponent::LuaInstance>(),
					MetaEntry<std::vector<Engine::LuaScriptComponent::LuaInstance>>()
				});
#endif
				return temp;
			}();
			return map;
		}

		const auto& GetJsonSetterMap()
		{
			using MapType = std::unordered_map<entt::id_type, JsonSetter>;
			static const MapType map = []()
			{
				MapType temp{
					JsonEntry<signed char>(),
					JsonEntry<short>(),
					JsonEntry<int>(),
					JsonEntry<long>(),
					JsonEntry<long long>(),
					JsonEntry<bool>(),
					JsonEntry<unsigned char>(),
					JsonEntry<unsigned short>(),
					JsonEntry<unsigned int>(),
					JsonEntry<unsigned long>(),
					JsonEntry<unsigned long long>(),
					JsonEntry<char>(),
					JsonEntry<wchar_t>(),
					JsonEntry<float>(),
					JsonEntry<double>(),
					JsonEntry<long double>(),
					JsonEntry<std::string>(),
					JsonEntry<std::nullptr_t>(),
					JsonEntry<char8_t>(),
					JsonEntry<char16_t>(),
					JsonEntry<char32_t>(),
					JsonEntry<glm::vec2>(),
					JsonEntry<glm::vec3>(),
					JsonEntry<glm::mat4>(),
					JsonEntry<xresource::full_guid>(),
					JsonEntry<std::vector<int>>(),
					JsonEntry<std::vector<float>>(),
					JsonEntry<std::vector<std::string>>(),
					JsonEntry<std::vector<AudioComponent::AudioClip>>(),
					JsonEntry<CameraComponent::CameraType>(),
					JsonEntry<LightComponent::Type>(),
					JsonEntry<std::filesystem::path>(),
					JsonEntry<TriggerShapeType>(),
					JsonEntry<::ColliderShape>(),
					JsonEntry<::ColliderData>(),
					JsonEntry<std::vector<::ColliderData>>(),
					JsonEntry<::ColliderComponent::Mode>(),
					JsonEntry<ColliderShape>(),
					JsonEntry<ColliderData>(),
					JsonEntry<std::vector<ColliderData>>(),
					JsonEntry<ColliderComponent::Mode>(),
					JsonEntry<xresource::instance_guid>(),
					JsonEntry<SoundBankMap>()
				};
#if defined(ENGINE_ENABLE_LUA)
				temp.insert({
					JsonEntry<Engine::LuaScriptComponent::LuaInstance>(),
					JsonEntry<std::vector<Engine::LuaScriptComponent::LuaInstance>>()
				});
#endif
				return temp;
			}();
			return map;
		}
	} // namespace detail

	// Templated class, to provide different function overloads for each type that all do the same thing
	template <typename T>
	class Emplacer
	{
	public:
		// Taking in entt::meta_any*, as references do not work
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

	// Templated class to remove components uniformly through entt::meta
	template <typename T>
	class Remover
	{
	public:
		static bool RemoveComponent(entt::entity entity)
		{
			if (!registry.all_of<T>(entity))
			{
				return false;
			}
			registry.remove<T>(entity);
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
			if constexpr (std::is_empty_v<T>)
			{
				return T{};
			}
			else return registry.get<T>(entity);
		}
	};

		// To return a meta_any that references the component in the registry
		template <typename T>
		static entt::meta_any GetRef(entt::entity e) {
			auto& reg = EngineFramework::GameObjectSystem::GetRegistry();
			if constexpr (std::is_empty_v<T>) {
				return entt::meta_any(std::in_place_type<T>);
			} else {
				return entt::forward_as_meta(reg.get<T>(e));
			}
		}
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
		REGISTER_ARBITRARY_TYPE(glm::vec2);
		REGISTER_ARBITRARY_TYPE(glm::vec3);
		REGISTER_ARBITRARY_TYPE(glm::mat4);
		REGISTER_ARBITRARY_TYPE(xresource::full_guid);
		REGISTER_ARBITRARY_TYPE(xresource::instance_guid);
		REGISTER_ARBITRARY_TYPE(std::vector<glm::vec3>);
		REGISTER_ARBITRARY_TYPE(CameraComponent::CameraType);
		REGISTER_ARBITRARY_TYPE(LightComponent::Type);
		REGISTER_ARBITRARY_TYPE(std::filesystem::path);
		REGISTER_ARBITRARY_TYPE(TriggerShapeType);
		REGISTER_ARBITRARY_TYPE(::ColliderShape);
		REGISTER_ARBITRARY_TYPE(ColliderShape);
		REGISTER_ARBITRARY_TYPE(::ColliderData);
		REGISTER_ARBITRARY_TYPE(ColliderData);
		REGISTER_ARBITRARY_TYPE(std::vector<::ColliderData>);
		REGISTER_ARBITRARY_TYPE(std::vector<ColliderData>);
		REGISTER_ARBITRARY_TYPE(::ColliderComponent::Mode);
		REGISTER_ARBITRARY_TYPE(ColliderComponent::Mode);
		REGISTER_ARBITRARY_TYPE(SoundBankMap);
#if defined(ENGINE_ENABLE_LUA)
		REGISTER_ARBITRARY_TYPE(Engine::LuaScriptComponent::LuaInstance);
		REGISTER_ARBITRARY_TYPE(std::vector<Engine::LuaScriptComponent::LuaInstance>);
#endif

		// Helper function to return a meta_any wrapping a reference

		// TODO - Register additional components (script, audio etc) and remember to tag them when registering (refer to TextureComponent)
		// === Register TransformComponent ===
		entt::meta_factory<TransformComponent>()
			.type("TransformComponent")
			.data<&TransformComponent::position>("position")
			.data<&TransformComponent::rotation>("rotation")
			.data<&TransformComponent::scale>("scale")
			.func<&Emplacer<TransformComponent>::EmplaceComponent>("emplace")
			.func<&Remover<TransformComponent>::RemoveComponent>("remove")
			.func<&Getter<TransformComponent>::GetComponent>("get")
			.func<&GetRef<TransformComponent>>("get_ref");

		// === Register MeshComponent ===
		entt::meta_factory<MeshComponent>()
			.type("MeshComponent")
			.data<&MeshComponent::meshName>("meshName")
			.func<&Emplacer<MeshComponent>::EmplaceComponent>("emplace")
			.func<&Remover<MeshComponent>::RemoveComponent>("remove")
			.func<&Getter<MeshComponent>::GetComponent>("get")
			.func<&GetRef<MeshComponent>>("get_ref");

		// === Register AnimationComponent ===
		entt::meta_factory<AnimationComponent>()
			.type("AnimationComponent")
			.data<&AnimationComponent::animationName>("animationName") 
			.data<&AnimationComponent::currentClip>("currentClip")
			.func<&Emplacer<AnimationComponent>::EmplaceComponent>("emplace")
			.func<&Remover<AnimationComponent>::RemoveComponent>("remove")
			.func<&Getter<AnimationComponent>::GetComponent>("get")
			.func<&GetRef<AnimationComponent>>("get_ref");

		// === Register MaterialComponent ===
		entt::meta_factory<MaterialComponent>()
			.type("MaterialComponent")
			.data<&MaterialComponent::name>("name")
			.data<&MaterialComponent::albedoMap>("albedoMap")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&MaterialComponent::normalMap>("normalMap")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&MaterialComponent::metallicMap>("metallicMap")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&MaterialComponent::roughnessMap>("roughnessMap")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&MaterialComponent::ambientOcclusionMap>("ambientOcclusionMap")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&MaterialComponent::albedoFactor>("albedoFactor")
			.data<&MaterialComponent::metallicFactor>("metallicFactor")
			.data<&MaterialComponent::roughnessFactor>("roughnessFactor")
			.func<&Emplacer<MaterialComponent>::EmplaceComponent>("emplace")
			.func<&Remover<MaterialComponent>::RemoveComponent>("remove")
			.func<&Getter<MaterialComponent>::GetComponent>("get")
			.func<&GetRef<MaterialComponent>>("get_ref");

		// === Register PhysicsMaterialComponent ===
		entt::meta_factory<PhysicsMaterialComponent>()
			.type("PhysicsMaterialComponent")
			.data<&PhysicsMaterialComponent::friction>("friction")
			.data<&PhysicsMaterialComponent::restitution>("restitution")
			.data<&PhysicsMaterialComponent::density>("density")
			.data<&PhysicsMaterialComponent::isStatic>("isStatic")
			.data<&PhysicsMaterialComponent::_prevIsStatic>("_prevIsStatic") // Expose internal tracking for now
			.data<&PhysicsMaterialComponent::layer>("layer")
			.func<&Emplacer<PhysicsMaterialComponent>::EmplaceComponent>("emplace")
			.func<&Remover<PhysicsMaterialComponent>::RemoveComponent>("remove")
			.func<&Getter<PhysicsMaterialComponent>::GetComponent>("get")
			.func<&GetRef<PhysicsMaterialComponent>>("get_ref");

		// === Register LightComponent ===
		entt::meta_factory<LightComponent>()
			.type("LightComponent")
			.data<&LightComponent::type>("type")
			.data<&LightComponent::position>("position")
			.data<&LightComponent::direction>("direction")
			.data<&LightComponent::color>("color")
			.data<&LightComponent::intensity>("intensity")
			.data<&LightComponent::radius>("radius")
			.data<&LightComponent::constant>("constant")
			.data<&LightComponent::linear>("linear")
			.data<&LightComponent::quadratic>("quadratic")
			.data<&LightComponent::innerConeAngle>("innerConeAngle")
			.data<&LightComponent::outerConeAngle>("outerConeAngle")
			.func<&Emplacer<LightComponent>::EmplaceComponent>("emplace")
			.func<&Remover<LightComponent>::RemoveComponent>("remove")
			.func<&Getter<LightComponent>::GetComponent>("get")
			.func<&GetRef<LightComponent>>("get_ref");


		// === Register CameraComponent ===
		// Tag assets that need needs a dropdown
		entt::meta_factory<CameraComponent>()
			.type("CameraComponent")
			.data<&CameraComponent::camType>("camType")
			.data<&CameraComponent::position>("position")
			.data<&CameraComponent::target>("target")
			.data<&CameraComponent::fieldOfView>("fieldOfView")
			.data<&CameraComponent::nearPlane>("nearPlane")
			.data<&CameraComponent::farPlane>("farPlane")
			.data<&CameraComponent::is_active>("is_active")
			.func<&Emplacer<CameraComponent>::EmplaceComponent>("emplace")
			.func<&Remover<CameraComponent>::RemoveComponent>("remove")
			.func<&Getter<CameraComponent>::GetComponent>("get")
			.func<&GetRef<CameraComponent>>("get_ref");

		// === Register AudioComponent ===
		//entt::meta_factory<AudioComponent>()
		//	.type("AudioComponent")
		//	.data<&AudioComponent::audio_guid>("audio_guid")
		//	.data<&AudioComponent::volume>("volume")
		//	.data<&AudioComponent::loop>("loop")
		//	.data<&AudioComponent::playOnStart>("playOnStart")
		//	.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Audio })
		//	.func<&Emplacer<AudioComponent>::EmplaceComponent>("emplace")
		//	.func<&Remover<AudioComponent>::RemoveComponent>("remove")
		//	.func<&Getter<AudioComponent>::GetComponent>("get")
		//	.func<&GetRef<AudioComponent>>("get_ref");

		// === Register AudioComponent ===
		entt::meta_factory<AudioComponent::AudioClip>()
			.type("AudioClip")
			.data<&AudioComponent::AudioClip::audio_guid>("audio_guid")
			.data<&AudioComponent::AudioClip::volume>("volume")
			.data<&AudioComponent::AudioClip::loop>("loop")
			.data<&AudioComponent::AudioClip::playOnStart>("playOnStart")
			.data<&AudioComponent::AudioClip::is3D>("is3D")
			.data<&AudioComponent::AudioClip::minDistance>("minDistance")
			.data<&AudioComponent::AudioClip::maxDistance>("maxDistance");

		entt::meta_factory<AudioComponent>()
			.type("AudioComponent")
			.data<&AudioComponent::Clips>("Clips")
			// We only save the Sound Bank now. The requestQueue is temporary and doesn't need saving.
			.data<&AudioComponent::soundBank>("soundBank")

			.data<&AudioComponent::playOnStartName>("playOnStartName")
			.data<&AudioComponent::playOnStartLoop>("playOnStartLoop")
			.data<&AudioComponent::playOnStart3D>("playOnStart3D")
			.data<&AudioComponent::playOnStartVolume>("playOnStartVolume")

			.func<&Emplacer<AudioComponent>::EmplaceComponent>("emplace")
			.func<&Remover<AudioComponent>::RemoveComponent>("remove")
			.func<&Getter<AudioComponent>::GetComponent>("get")
			.func<&GetRef<AudioComponent>>("get_ref");

		// === Register AudioListenerComponent ===
		entt::meta_factory<AudioListenerComponent>()
			.type("AudioListenerComponent")
			.data<&AudioListenerComponent::active>("active")
			.func<&Emplacer<AudioListenerComponent>::EmplaceComponent>("emplace")
			.func<&Remover<AudioListenerComponent>::RemoveComponent>("remove")
			.func<&Getter<AudioListenerComponent>::GetComponent>("get")
			.func<&GetRef<AudioListenerComponent>>("get_ref");

		entt::meta_factory<::ColliderShape>()
			.type("ColliderShape") // Arbitrary name for the type
			/*.data<ColliderShape::Box>("Box"_hs)
			.data<ColliderShape::Sphere>("Sphere"_hs)
			.data<ColliderShape::Capsule>("Capsule"_hs)*/
			.conv<std::underlying_type_t<::ColliderShape>>();

		entt::meta_factory<::ColliderData>()
			.type("ColliderData")
			.ctor<>()
			// Dimensions
			.data<&::ColliderData::type>("type")
			.data<&ColliderData::boxHalfExtents>("boxHalfExtents")
			.data<&ColliderData::sphereRadius>("sphereRadius")
			.data<&ColliderData::capsuleHeight>("capsuleHeight")
			.data<&ColliderData::capsuleRadius>("capsuleRadius")
			// Offsets
			.data<&ColliderData::offsetPosition>("offsetPosition")
			.data<&ColliderData::offsetRotation>("offsetRotation");

		entt::meta_factory<::ColliderComponent::Mode>()
			.type("ColliderMode")
			/*.data<ColliderComponent::Mode::SOLID_COLLIDER>("SOLID_COLLIDER"_hs)
			.data<ColliderComponent::Mode::TRIGGER_SENSOR>("TRIGGER_SENSOR"_hs)
			.data<ColliderComponent::Mode::LEVEL_EDITOR>("LEVEL_EDITOR"_hs)*/
			.conv<std::underlying_type_t<::ColliderComponent::Mode>>();

		// === Register ColliderComponent ===
		entt::meta_factory<ColliderComponent>()
			.type("ColliderComponent")
			.data<&::ColliderComponent::mode>("mode")
			.data<&ColliderComponent::shapes>("shapes")
			.data<&ColliderComponent::minBounds>("minBounds") // AABB
			.data<&ColliderComponent::maxBounds>("maxBounds") // AABB
			.func<&Emplacer<ColliderComponent>::EmplaceComponent>("emplace")
			.func<&Remover<ColliderComponent>::RemoveComponent>("remove")
			.func<&Getter<ColliderComponent>::GetComponent>("get")
			.func<&GetRef<ColliderComponent>>("get_ref");

		entt::meta_factory<PhysicsComponent>()
			.type("PhysicsComponent")
			.data<&PhysicsComponent::velocity>("velocity")
			.data<&PhysicsComponent::acceleration>("acceleration")
			.data<&PhysicsComponent::mass>("mass")
			// Keep runtime-specific gravity control
			.data<&PhysicsComponent::useGravity>("useGravity")
			.data<&PhysicsComponent::gravityFactor>("gravityFactor")
			.data<&PhysicsComponent::_prevUseGravity>("_prevUseGravity") // Expose internal tracking
			.data<&PhysicsComponent::editorOverride>("editorOverride")
			.func<&Emplacer<PhysicsComponent>::EmplaceComponent>("emplace")
			.func<&Remover<PhysicsComponent>::RemoveComponent>("remove")
			.func<&Getter<PhysicsComponent>::GetComponent>("get")
			.func<&GetRef<PhysicsComponent>>("get_ref");

		// === Register LuaScriptComponent and nested LuaInstance ===

		// First, register the inner LuaInstance struct
		entt::meta_factory<Engine::LuaScriptComponent::LuaInstance>()
			.type("LuaInstance")
			.data<&Engine::LuaScriptComponent::LuaInstance::ScriptGuid>("ScriptGuid")
			.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Script })
			.data<&Engine::LuaScriptComponent::LuaInstance::Loaded>("Loaded");

		// Then register the main LuaScriptComponent
		entt::meta_factory<Engine::LuaScriptComponent>()
			.type("LuaScriptComponent")
			.data<&Engine::LuaScriptComponent::Enabled>("Enabled")
			.data<&Engine::LuaScriptComponent::Scripts>("Scripts")
			.func<&Emplacer<Engine::LuaScriptComponent>::EmplaceComponent>("emplace")
			.func<&Remover<Engine::LuaScriptComponent>::RemoveComponent>("remove")
			.func<&Getter<Engine::LuaScriptComponent>::GetComponent>("get")
			.func<&GetRef<Engine::LuaScriptComponent>>("get_ref");

		// Tag components (if you need them for serialization)
		// Register SceneRenderPass
		entt::meta_factory<SceneRenderPass>()
			.type("SceneRenderPass")
			.func<&Emplacer<SceneRenderPass>::EmplaceComponent>("emplace")
			.func<&Remover<SceneRenderPass>::RemoveComponent>("remove")
			.func<&Getter<SceneRenderPass>::GetComponent>("get")
			.func<&GetRef<SceneRenderPass>>("get_ref")
			.custom<RequiresSerializedHistoryTag>(RequiresSerializedHistoryTag{});

		// Register SkyboxRenderPass
		entt::meta_factory<SkyboxRenderPass>()
			.type("SkyboxRenderPass")
			.func<&Emplacer<SkyboxRenderPass>::EmplaceComponent>("emplace")
			.func<&Remover<SkyboxRenderPass>::RemoveComponent>("remove")
			.func<&Getter<SkyboxRenderPass>::GetComponent>("get")
			.func<&GetRef<SkyboxRenderPass>>("get_ref")
			.custom<RequiresSerializedHistoryTag>(RequiresSerializedHistoryTag{});

		entt::meta_factory<TriggerBoxComponent>()
			.type("TriggerBoxComponent")
			.data<&TriggerBoxComponent::enabled>("enabled")
			.data<&TriggerBoxComponent::type>("type")
			.data<&TriggerBoxComponent::halfExtents>("halfExtents")
			.data<&TriggerBoxComponent::radius>("radius")
			.data<&TriggerBoxComponent::capHalfHeight>("capHalfHeight")
			.data<&TriggerBoxComponent::capRadius>("capRadius")
			.data<&TriggerBoxComponent::localPosition>("localPosition")
			.data<&TriggerBoxComponent::localEulerDeg>("localEulerDeg")
			.data<&TriggerBoxComponent::debugColor>("debugColor")
			// runtime fields are typically NOT serialized; omit if you don�t want them saved:
			//.data<&TriggerBoxComponent::bodyId>("bodyId")
			//.data<&TriggerBoxComponent::isRegistered>("isRegistered")
			.func<&Emplacer<TriggerBoxComponent>::EmplaceComponent>("emplace")
			.func<&Remover<TriggerBoxComponent>::RemoveComponent>("remove")
			.func<&Getter<TriggerBoxComponent>::GetComponent>("get")
			.func<&GetRef<TriggerBoxComponent>>("get_ref");

		entt::meta_factory<UIComponent>()
			.type("UIComponent") 
			.data<&UIComponent::textureid>("textureid")
				.custom<EditorFeatures::AssetPickerTag>(EditorFeatures::AssetPickerTag{ EditorFeatures::AssetPickerKind::Texture })
			.data<&UIComponent::position>("position")
			.data<&UIComponent::size>("size")
			.data<&UIComponent::color>("color")
			.data<&UIComponent::visible>("visible")
			.data<&UIComponent::luminance>("luminance")
			.data<&UIComponent::layer>("layer")
			.func<&Emplacer<UIComponent>::EmplaceComponent>("emplace")
			.func<&Remover<UIComponent>::RemoveComponent>("remove")
			.func<&Getter<UIComponent>::GetComponent>("get")
			.func<&GetRef<UIComponent>>("get_ref");

		// Register the name component
		entt::meta_factory<NameComponent>()
			.type("NameComponent")
			.data<&NameComponent::name>("name")
			.func<&Emplacer<NameComponent>::EmplaceComponent>("emplace")
			.func<&Remover<NameComponent>::RemoveComponent>("remove")
			.func<&Getter<NameComponent>::GetComponent>("get")
			.func<&GetRef<NameComponent>>("get_ref");

	}

	//Helper function to cast json to the correct data type, and set comp_data.
	void SetMetaAnyWithJson(entt::meta_any& comp_data, const entt::meta_data& data_type, const nlohmann::json& data)
	{
		if (!data_type) return;
		const entt::id_type type = data_type.type().id();
		const auto& setters = detail::GetMetaSetterMap();
		if (auto it = setters.find(type); it != setters.end())
		{
			it->second(comp_data, data_type, data);
			return;
		}

		assert(false && "Type is not supported by SetMetaAnyWithJson function. Please add to map.");
	}

	// Helper function to cast meta_any member_data to the correct json data type, and set comp_json.
	void SetJsonWithMetaAny(nlohmann::json& comp_json, const entt::meta_data& data_type, const entt::meta_any& data)
	{
		if (!data_type) return;

		std::string name = data_type.name();
		if (name.empty())
		{
			throw std::runtime_error("Meta Data does not have a name, was likely registered with a _hs hashed string instead of a string.");
		}

		const entt::id_type type = data_type.type().id();
		const auto& setters = detail::GetJsonSetterMap();
		if (auto it = setters.find(type); it != setters.end())
		{
			it->second(comp_json, name, data);
			return;
		}

		assert(false && "Type is not supported by SetJsonWithMetaAny function. Please add to map.");
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
}
