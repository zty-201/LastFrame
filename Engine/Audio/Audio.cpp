/******************************************************************************/
/*!
\file		Audio.cpp
\author 	Mohamed Arshad Bin Mohamed Hussain
\par    	mohamedarshad.b\@digipen.edu
\brief  	Audio System for the Cheerios Engine

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#include "pch.hpp"
#include "Audio.hpp"

#include "SoundAsset.hpp"
#include "System_GameObjectSystem.hpp"
#include "TransformComponent.h"
#include "AudioComponent.h"
#include "AudioListenerComponent.h"
#include "AssetManager.hpp"

#include <entt/entt.hpp>
#include <algorithm>
#include <iostream>

#if defined(_DEBUG) && defined(PLATFORM_WINDOWS)
#include "System_LevelEditor.hpp"
#endif

//#ifdef PLATFORM_ANDROID
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#endif

namespace EngineFramework
{
	
	AudioSystem* AudioSystem::this_system = nullptr;

#ifndef DISABLE_FMOD
	void AudioSystem::logFmodError(FMOD_RESULT result, const char* context)
	{
		if (result != FMOD_OK)
		{
			std::cerr << "[AudioSystem] FMOD error in " << context
				<< ": (" << static_cast<int>(result) << ") "
				<< FMOD_ErrorString(result) << "\n";
		}
	}

	FMOD_MODE AudioSystem::modeFor(bool looped, bool is3D, AudioRolloff rolloff) const
	{
		FMOD_MODE mode = FMOD_DEFAULT;
		mode |= looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
		mode |= is3D ? FMOD_3D : FMOD_2D;

		switch (rolloff)
		{
		case AudioRolloff::Linear:  mode |= FMOD_3D_LINEARROLLOFF;  break;
		case AudioRolloff::Inverse: mode |= FMOD_3D_INVERSEROLLOFF; break;
		}
		return mode;
	}

	FMOD::ChannelGroup* AudioSystem::pickGroup(AudioGroup group)
	{
		switch (group)
		{
		case AudioGroup::BGM:    return m_groupBGM;
		case AudioGroup::SFX:      return m_groupSFX;
		case AudioGroup::UI:       return m_groupUI;
		case AudioGroup::Ambience: return m_groupAmbience;
		case AudioGroup::Master:

		default:                   return m_groupMaster;
		}
	}
#endif

	AudioSystem::AudioSystem()
	{
		this_system = this;
	}

	AudioSystem::~AudioSystem()
	{
#ifndef DISABLE_FMOD
		if (m_ambienceLowpass)		{ m_ambienceLowpass->release();			m_ambienceLowpass = nullptr; }
		if (m_groupAmbienceOutdoor) { m_groupAmbienceOutdoor->release();	m_groupAmbienceOutdoor = nullptr; }
		if (m_groupAmbienceIndoor)	{ m_groupAmbienceIndoor->release();		m_groupAmbienceIndoor = nullptr; }
		if (m_groupAmbience)		{ m_groupAmbience->release();			m_groupAmbience = nullptr; }
		if (m_groupUI)				{ m_groupUI->release();					m_groupUI = nullptr; }
		if (m_groupBGM)				{ m_groupBGM->release();				m_groupBGM = nullptr; }
		if (m_groupSFX)				{ m_groupSFX->release();				m_groupSFX = nullptr; }
		if (m_groupMaster)			{ m_groupMaster->release();				m_groupMaster = nullptr; }

		if (m_system)
		{
			m_system->close();
			m_system->release();
			m_system = nullptr;
		}
#endif
	}

	void AudioSystem::Initialize()
	{
		if (m_initialized) return;

#ifndef DISABLE_FMOD
		FMOD_RESULT r;

		r = FMOD::System_Create(&m_system);
		logFmodError(r, "System_Create");
		if (r != FMOD_OK || !m_system) return;

		r = m_system->init(64, FMOD_INIT_NORMAL, nullptr);
		logFmodError(r, "System::init");
		if (r != FMOD_OK) return;

		r = m_system->set3DSettings(1.0f, 1.0f, 0.35f);
		logFmodError(r, "System::set3DSettings");
		if (r != FMOD_OK) return;

		// Create channel groups
		r = m_system->createChannelGroup("Master", &m_groupMaster);     logFmodError(r, "createChannelGroup(Master)");     if (r != FMOD_OK) return;
		r = m_system->createChannelGroup("Music", &m_groupBGM);       logFmodError(r, "createChannelGroup(Music)");      if (r != FMOD_OK) return;
		r = m_system->createChannelGroup("SFX", &m_groupSFX);           logFmodError(r, "createChannelGroup(SFX)");        if (r != FMOD_OK) return;
		r = m_system->createChannelGroup("UI", &m_groupUI);             logFmodError(r, "createChannelGroup(UI)");         if (r != FMOD_OK) return;
		r = m_system->createChannelGroup("Ambience", &m_groupAmbience); logFmodError(r, "createChannelGroup(Ambience)");   if (r != FMOD_OK) return;

		r = m_system->createChannelGroup("AmbienceOutdoor", &m_groupAmbienceOutdoor); logFmodError(r, "createChannelGroup(AmbienceOutdoor)");   if (r != FMOD_OK) return;
		r = m_system->createChannelGroup("AmbienceIndoor", &m_groupAmbienceIndoor); logFmodError(r, "createChannelGroup(AmbienceIndoor)");   if (r != FMOD_OK) return;

		// Attach to master
		if (m_groupMaster)
		{
			if (m_groupBGM)
			{
				FMOD_RESULT rr = m_groupMaster->addGroup(m_groupBGM);
				logFmodError(rr, "Master->addGroup(BGM)");
			}
			if (m_groupSFX)
			{
				FMOD_RESULT rr = m_groupMaster->addGroup(m_groupSFX);
				logFmodError(rr, "Master->addGroup(SFX)");
			}
			if (m_groupUI)
			{
				FMOD_RESULT rr = m_groupMaster->addGroup(m_groupUI);
				logFmodError(rr, "Master->addGroup(UI)");
			}
			if (m_groupAmbience)
			{
				FMOD_RESULT rr = m_groupMaster->addGroup(m_groupAmbience);
				logFmodError(rr, "Master->addGroup(Ambience)");

				if (m_groupAmbienceOutdoor)
				{
					rr = m_groupAmbience->addGroup(m_groupAmbienceOutdoor);
					logFmodError(rr, "Ambience->addGroup(AmbienceOutdoor)");
				}
				if (m_groupAmbienceIndoor)
				{
					rr = m_groupAmbience->addGroup(m_groupAmbienceIndoor);
					logFmodError(rr, "Ambience->addGroup(AmbienceIndoor)");
				}
			}
		}

		// Default volumes
		if (m_groupMaster)   m_groupMaster->setVolume(1.0f);
		if (m_groupBGM)      m_groupBGM->setVolume(1.0f);
		if (m_groupSFX)      m_groupSFX->setVolume(1.0f);
		if (m_groupUI)       m_groupUI->setVolume(1.0f);
		if (m_groupAmbience) m_groupAmbience->setVolume(1.0f);
		if (m_groupAmbienceOutdoor) m_groupAmbienceOutdoor->setVolume(1.0f);
		if (m_groupAmbienceIndoor) m_groupAmbienceIndoor->setVolume(1.0f);

		// Ambience muffling DSP (lowpass), controlled by SetIndoorFactor()
		if (m_system && m_groupAmbienceOutdoor)
		{
			FMOD_RESULT r2 = m_system->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &m_ambienceLowpass);
			logFmodError(r2, "createDSPByType(LOWPASS)");

			if (r2 == FMOD_OK && m_ambienceLowpass)
			{
				r2 = m_groupAmbienceOutdoor->addDSP(0, m_ambienceLowpass);
				logFmodError(r2, "Ambience addDSP(LOWPASS)");

				// Start outdoors: no muffling
				m_ambienceLowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 20000.0f);
				m_ambienceLowpass->setBypass(true);
			}
		}

#endif

		m_initialized = true;
		LOGI("%s", "[AudioSystem] Initialization Successful!");

		m_inSimulation = true;

	}

	void AudioSystem::SetPathResolver(std::function<std::string(const std::string&)> resolver)
	{
		m_pathResolver = resolver;
	}

	std::string AudioSystem::ResolvePath(const std::string& logicalOrPath) const
	{
		return m_pathResolver ? m_pathResolver(logicalOrPath) : logicalOrPath;
	}

	// -----------------------------
	// Enqueue commands
	// -----------------------------
	void AudioSystem::PlayEntitySlot(entt::entity entity, const std::string& slotId)
	{
		m_commands.push_back({ AudioCommand::Type::PlaySlot, entity, slotId });
	}

	void AudioSystem::StopEntitySlot(entt::entity entity, const std::string& slotId)
	{
		m_commands.push_back({ AudioCommand::Type::StopSlot, entity, slotId });
	}

	void AudioSystem::StopAllForEntity(entt::entity entity)
	{
		m_commands.push_back({ AudioCommand::Type::StopAllForEntity, entity, {} });
	}

	EngineFramework::AudioGroup EngineFramework::AudioSystem::MapCategoryToGroup(AudioCategory category) const
	{
		switch (category)
		{
		case AudioCategory::BGM:    return AudioGroup::BGM;
		case AudioCategory::UI:       return AudioGroup::UI;
		case AudioCategory::Ambience: return AudioGroup::Ambience;
		case AudioCategory::SFX:
		default:                                      return AudioGroup::SFX;
		}
	}

	void AudioSystem::Update(float dt)
	{
		(void)dt;
#ifndef DISABLE_FMOD
		if (!m_initialized || !m_system) return;
#endif

		auto& ecs = EngineFramework::GameObjectSystem::GetRegistry();

		// 1) Listener update (keep your logic)
#ifndef DISABLE_FMOD
		{
			auto viewListener = ecs.view<TransformComponent, AudioListenerComponent>();
			bool listenerSet = false;

			viewListener.each([&](entt::entity, TransformComponent& tf, AudioListenerComponent& listener)
				{
					if (listenerSet || !listener.active) return;

					glm::vec3 pos = tf.position;
					glm::quat q = glm::quat(glm::radians(tf.rotation));
					glm::vec3 forward = q * glm::vec3(0.0f, 0.0f, -1.0f);
					glm::vec3 up = q * glm::vec3(0.0f, 1.0f, 0.0f);
					glm::vec3 vel(0.0f);

					FMOD_VECTOR fPos{ pos.x, pos.y, pos.z };
					FMOD_VECTOR fVel{ vel.x, vel.y, vel.z };
					FMOD_VECTOR fFwd{ forward.x, forward.y, forward.z };
					FMOD_VECTOR fUp{ up.x, up.y, up.z };

					m_system->set3DListenerAttributes(0, &fPos, &fVel, &fFwd, &fUp);
					listenerSet = true;

					//LOGI("[Audio] Listener rot = (%.3f, %.3f, %.3f)", tf.rotation.x, tf.rotation.y, tf.rotation.z);
				});
		}
#endif

		// 2) playOnStart slots
#if defined(_DEBUG) && defined(PLATFORM_WINDOWS)
		if (!LevelEditorSystem::Get_Enable() && m_inSimulation)
			HandlePlayOnStart(ecs);
#else
		if (m_inSimulation)
			HandlePlayOnStart(ecs);
#endif

		// 3) Process queued play/stop commands
		ProcessCommands(ecs);

		// 4) Update looping 3D instances to follow entity transforms
		//if(m_inSimulation)
		UpdateActiveInstances(ecs, dt);

#ifndef DISABLE_FMOD
		// 5) FMOD internal update
		auto r = m_system->update();

		// Cleanup playOnStart guard for destroyed entities (prevents entity-id reuse bugs)
		for (auto it = m_playOnStartDone.begin(); it != m_playOnStartDone.end(); )
		{
			if (!ecs.valid(it->first))
				it = m_playOnStartDone.erase(it);
			else
				++it;
		}

		logFmodError(r, "System::update");
#endif
	}

	void AudioSystem::PostUpdate(float dt)
	{
		(void)dt;
	}

	void AudioSystem::HandlePlayOnStart(entt::registry& ecs)
	{
		static bool printedOnce = false;
		if (!printedOnce)
		{
			LOGI("[Audio] HandlePlayOnStart() is running");
			printedOnce = true;
		}

		auto view = ecs.view<AudioComponent>();
		view.each([&](entt::entity e, AudioComponent& comp)
			{
				//LOGI("[Audio] Found AudioComponent on entity %u, Slots=%zu", (uint32_t)e, comp.Slots.size());

				if (m_playOnStartDone[e]) return;

				for (const auto& slot : comp.Slots)
				{
					if (slot.playOnStart && !slot.id.empty())
					{
						//LOGI("[Audio] Queuing playOnStart slot '%s' for entity %u", slot.id.c_str(), (uint32_t)e);

						PlayEntitySlot(e, slot.id);
					}
				}

				m_playOnStartDone[e] = true;
			});
	}

	void AudioSystem::ProcessCommands(entt::registry& ecs)
	{
		for (const auto& cmd : m_commands)
		{
			switch (cmd.type)
			{
			case AudioCommand::Type::PlaySlot:        ProcessPlaySlot(ecs, cmd); break;
			case AudioCommand::Type::StopSlot:        ProcessStopSlot(cmd); break;
			case AudioCommand::Type::StopAllForEntity:ProcessStopAllForEntity(cmd); break;
			}
		}
		m_commands.clear();
	}

	void AudioSystem::ProcessPlaySlot(entt::registry& ecs, const AudioCommand& cmd)
	{
		if (!ecs.valid(cmd.entity)) return;
		if (!ecs.all_of<AudioComponent, TransformComponent>(cmd.entity)) return;

		auto& comp = ecs.get<AudioComponent>(cmd.entity);
		auto& tf = ecs.get<TransformComponent>(cmd.entity);

		const auto* slot = comp.FindSlot(cmd.slotId);
		if (!slot) return;

#ifndef DISABLE_FMOD
		// Load sound via AssetManager GUID (your existing approach)
		using EngineFramework::AssetManagerSystem;
		std::shared_ptr<SoundAsset> sound = AssetManagerSystem::GetResource<SoundAsset>(slot->audio_guid);
		if (!sound || !sound->sound) return;

		FMOD::Channel* channel = nullptr;
		auto r = m_system->playSound(sound->sound, nullptr, true, &channel);
		logFmodError(r, "System::playSound (PlaySlot)");
		if (r != FMOD_OK || !channel) return;

		// Route to category bus
		AudioGroup group = MapCategoryToGroup(slot->category);
		if (group == AudioGroup::Ambience)
		{
			switch (slot->ambienceType)
			{
				case AudioComponent::AmbienceType::Outdoor:
					if (m_groupAmbienceOutdoor) channel->setChannelGroup(m_groupAmbienceOutdoor);
					break;
				case AudioComponent::AmbienceType::Indoor:
					if (m_groupAmbienceIndoor) channel->setChannelGroup(m_groupAmbienceIndoor);
					break;
				default:
					if (m_groupAmbience) channel->setChannelGroup(m_groupAmbience);
					break;
			}
		}

		// Fade
		float finalTargetVolume = std::clamp(slot->volume, 0.0f, 5.0f);

		if (slot->useFade)
		{
			channel->setVolume(0.0f);	// Start at 0.0 so it will ramp up
		}
		else
		{
			channel->setVolume(finalTargetVolume);	// If no fade, set to desired volume
		}

		//channel->setVolume(std::clamp(slot->volume, 0.0f, 1.0f));
		channel->setPitch(slot->pitch);

		const bool is3D = (slot->spatial == AudioComponent::SpatialMode::ThreeD);
		FMOD_MODE channelMode = (slot->loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		channelMode |= is3D ? (FMOD_3D | FMOD_3D_LINEARROLLOFF) : FMOD_2D;
		channel->setMode(channelMode);

		if (is3D)
		{
			glm::vec3 pos = tf.position;
			FMOD_VECTOR fPos{ pos.x, pos.y, pos.z };
			FMOD_VECTOR fVel{ 0.0f, 0.0f, 0.0f };
			channel->set3DAttributes(&fPos, &fVel);
			channel->set3DMinMaxDistance(slot->minDistance, slot->maxDistance);
		}

		channel->setPaused(false);

		// Track only if looping or if you want follow-up control by slot
		//if (slot->loop || is3D)
		//{
			// If an instance with same entity+slot exists, stop it and replace (optional policy)
			for (auto& inst : m_active)
			{
				if (inst.entity == cmd.entity && inst.slotId == cmd.slotId)
				{
					if (inst.isFadingOut) continue;

					if (inst.useFade && inst.fadeDuration > 0.0f)
					{
						//LOGI("[Audio] Marking isFadingOut=true | Slot: %s | Triggered by PlaySlot command for: %s", inst.slotId.c_str(), cmd.slotId.c_str());
						inst.isFadingOut = true;
					}
					else
					{
						if (inst.channel) inst.channel->stop();
						inst.channel = nullptr;
					}

					//if (inst.channel) inst.channel->stop();
					//inst = {}; // clear
				}
			}

			m_active.push_back({
				cmd.entity,
				cmd.slotId,
				channel,
				slot->useFade,					// Designer's choice to fade or not
				slot->fadeDuration,
				slot->useFade ? 0.0f : 1.0f,	// Current fade volume (start at 0 if fade, 1 if not)
				finalTargetVolume,				// Ramp up to this volume
				false,							// isFadingOut
				is3D,
				slot->loop,
				slot->minDistance,
				slot->maxDistance
				});
		//}
#else
		(void)tf; (void)slot;
#endif
	}

	void AudioSystem::ProcessStopSlot(const AudioCommand& cmd)
	{
#ifndef DISABLE_FMOD
		//LOGI("[Audio] ProcessStopSlot called | Slot: %s", cmd.slotId.c_str());
		for (auto& inst : m_active)
		{
			if (inst.entity == cmd.entity && inst.slotId == cmd.slotId)
			{

				if (inst.isFadingOut) continue;	// Let it fade

				if (inst.useFade && inst.fadeDuration > 0.0f)
				{
					//LOGI("[Audio] Marking isFadingOut=true | Slot: %s | Triggered by StopSlot", inst.slotId.c_str());
					inst.isFadingOut = true;
				}
				else
				{
					if (inst.channel) inst.channel->stop();
					inst.channel = nullptr;
				}
			}
		}
#endif
	}

	void AudioSystem::ProcessStopAllForEntity(const AudioCommand& cmd)
	{
#ifndef DISABLE_FMOD
		for (auto& inst : m_active)
		{
			if (inst.entity == cmd.entity)
			{
				if (inst.channel) inst.channel->stop();
				inst.channel = nullptr;
			}
		}
#endif
	}

	void AudioSystem::UpdateActiveInstances(entt::registry& ecs, float dt)
	{
#ifndef DISABLE_FMOD
		// Remove dead/stopped channels and update 3D positions
		for (auto& instance : m_active)
		{
			if (!instance.channel) continue;

			// 1. Calculate Fade Logic if useFade is enabled
			if (instance.useFade && instance.fadeDuration > 0.0f)
			{
				float fadeStep = dt / instance.fadeDuration;

				if (instance.isFadingOut)
				{
					instance.currentFadeVolume -= fadeStep;

					// Print during fade-out
					//LOGI("[Audio] Fading OUT | Slot: %s | Multiplier: %.2f", instance.slotId.c_str(), instance.currentFadeVolume);

					if (instance.currentFadeVolume <= 0.0f)
					{
						instance.currentFadeVolume = 0.0f;
						instance.channel->stop();
						instance.channel = nullptr;
						continue;
					}
				}
				else if (instance.currentFadeVolume < 1.0f)
				{
					instance.currentFadeVolume = std::min(1.0f, instance.currentFadeVolume + fadeStep);

					// Print during fade-in
					//LOGI("[Audio] Fading IN | Slot: %s | Multiplier: %.2f", instance.slotId.c_str(), instance.currentFadeVolume);
				}
			}
			else
			{
				// 2. If no fade enabled
				instance.currentFadeVolume = 1.0f;
			}

			// 3. Apply volume
			if (instance.isFadingOut || instance.currentFadeVolume < 1.0f)
			{
				float equalPowerMultiplier = std::sin(instance.currentFadeVolume * (3.14159265f * 0.5f));
				float finalVolume = instance.targetVolume * equalPowerMultiplier;
				instance.channel->setVolume(finalVolume);
			}

			// 4. Existing Entity & 3D Validation
			if (!ecs.valid(instance.entity) || !ecs.all_of<TransformComponent>(instance.entity))
			{
				instance.channel->stop();
				instance.channel = nullptr;
				continue;
			}

			// 5. Remove if FMOD stopped it naturally (non-looping sounds that finished)
			// BUT: do not remove looping channels - they should play forever until explicitly stopped
			if (!instance.looping)
			{
				bool playing = false;
				if (instance.channel->isPlaying(&playing) != FMOD_OK || !playing)
				{
					instance.channel = nullptr;
					continue;
				}
			}

			if (instance.is3D)
			{
				auto& tf = ecs.get<TransformComponent>(instance.entity);

				FMOD_VECTOR fPos{ tf.position.x, tf.position.y, tf.position.z };
				FMOD_VECTOR fVel{ 0.0f, 0.0f, 0.0f };
				instance.channel->set3DAttributes(&fPos, &fVel);
				instance.channel->set3DMinMaxDistance(instance.minDistance, instance.maxDistance);
			}
		}

		// Compact
		m_active.erase(std::remove_if(m_active.begin(), m_active.end(),
			[](const ActiveInstance& a) { return a.channel == nullptr; }),
			m_active.end());
#else
		(void)ecs;
		(void)dt;
#endif
	}

	std::shared_ptr<SoundAsset> AudioSystem::Load(const std::string& logicalOrPath, bool loop)
	{
		if (!m_initialized)
			throw std::runtime_error("Audio System Not Initialized");

		const std::string resolved = ResolvePath(logicalOrPath);

#ifndef DISABLE_FMOD
		FMOD::Sound* snd = nullptr;
		FMOD_MODE mode = modeFor(loop, true, AudioRolloff::Linear);

#ifdef PLATFORM_ANDROID
		std::vector<unsigned char> file_data = AssetManagerSystem::GetDataFromFile(resolved);
		if (file_data.empty())
			throw std::runtime_error((std::string("Unable to read Audio file: ") + resolved).c_str());

		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.length = file_data.size();

		FMOD_RESULT r = m_system->createSound(reinterpret_cast<const char*>(file_data.data()), mode | FMOD_OPENMEMORY, &exinfo, &snd);
#else
		FMOD_RESULT r = m_system->createSound(resolved.c_str(), mode, nullptr, &snd);
#endif
		logFmodError(r, "System::createSound");
		if (r != FMOD_OK || !snd)
			throw std::runtime_error("Audio Not Loaded");

		std::shared_ptr<SoundAsset> info = std::make_shared<SoundAsset>();
		info->sound = snd;
		info->looped = loop;
		return info;
#else
		(void)resolved; (void)loop;
		return std::make_shared<SoundAsset>();
#endif
	}

	bool AudioSystem::Play(const std::shared_ptr<SoundAsset>& sound, float volume, bool paused, AudioGroup group)
	{
		if (!m_initialized) return false;

#ifndef DISABLE_FMOD
		if (!m_system || !sound || !sound->sound) return false;

		FMOD::Channel* channel = nullptr;
		FMOD_RESULT r = m_system->playSound(sound->sound, nullptr, true, &channel);
		logFmodError(r, "System::playSound");
		if (r != FMOD_OK || !channel) return false;

		if (auto* grp = pickGroup(group)) channel->setChannelGroup(grp);
		channel->setVolume(std::clamp(volume, 0.0f, 5.0f));
		channel->setPaused(paused ? true : false);
		if (!paused) channel->setPaused(false);
		return true;
#else
		(void)sound; (void)volume; (void)paused; (void)group;
		return true;
#endif
	}

	bool AudioSystem::PlayOneShot(const std::string& guid,
		const glm::vec3& worldPos,
		float volume,
		bool is3D,
		bool loop,
		float minDist,
		float maxDist,
		AudioGroup group)
	{
		if (!m_initialized || m_system == nullptr) return false;

#ifndef DISABLE_FMOD
		using EngineFramework::AssetManagerSystem;
		auto fullID = AssetManagerSystem::GetFullGuid(AssetManagerSystem::AUDIO_GUID, guid);
		std::shared_ptr<SoundAsset> sound = AssetManagerSystem::GetResource<SoundAsset>(fullID);
		if (!sound || !sound->sound) return false;

		FMOD::Channel* channel = nullptr;
		FMOD_RESULT r = m_system->playSound(sound->sound, nullptr, true, &channel);
		logFmodError(r, "System::playSound (PlayOneShot)");
		if (r != FMOD_OK || !channel) return false;

		if (auto* grp = pickGroup(group)) channel->setChannelGroup(grp);
		channel->setVolume(std::clamp(volume, 0.0f, 5.0f));
		channel->setMode((loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | (is3D ? FMOD_3D : FMOD_2D));

		if (is3D)
		{
			FMOD_VECTOR pos{ worldPos.x, worldPos.y, worldPos.z };
			FMOD_VECTOR vel{ 0.0f, 0.0f, 0.0f };
			channel->set3DAttributes(&pos, &vel);
			channel->set3DMinMaxDistance(minDist, maxDist);
		}

		channel->setPaused(false);
		return true;
#else
		(void)guid; (void)worldPos; (void)volume; (void)is3D; (void)loop; (void)minDist; (void)maxDist; (void)group;
		return true;
#endif
	}

	void AudioSystem::StopAll()
	{
		if (!m_initialized) return;
#ifndef DISABLE_FMOD

		// Stop all tracked active instances explicitly
		for (auto& inst : m_active)
		{
			if (inst.channel)
			{
				inst.channel->stop();
				inst.channel = nullptr;
			}
		}

		m_active.clear();

		if (m_groupMaster) m_groupMaster->stop();
#endif
	}

	void AudioSystem::PauseAll(bool paused)
	{
		if (!m_initialized) return;
#ifndef DISABLE_FMOD
		if (m_groupMaster) m_groupMaster->setPaused(paused);
#endif
	}

	void AudioSystem::SetMasterVolume(float volume)
	{
		if (!m_initialized) return;
		volume = std::clamp(volume, 0.0f, 5.0f);
#ifndef DISABLE_FMOD
		if (m_groupMaster) m_groupMaster->setVolume(volume);
#endif
	}

	void AudioSystem::SetGroupVolume(AudioGroup group, float volume)
	{
		if (!m_initialized) return;
		volume = std::clamp(volume, 0.0f, 5.0f);
#ifndef DISABLE_FMOD
		if (auto* grp = pickGroup(group)) grp->setVolume(volume);
#endif
	}

	bool AudioSystem::IsInitialized() const
	{
		return m_initialized;
	}

	void AudioSystem::SetSimulationMode(bool enabled)
	{
		//LOGI("[Audio] SetSimulationMode called | enabled=%d | m_inSimulation=%d", enabled, m_inSimulation);
		
		if (m_inSimulation == enabled)
			return;

		m_inSimulation = enabled;

		if (m_inSimulation)
		{
			// Entering simulator: allow playOnStart to trigger again
			m_playOnStartDone.clear();
		}
		else
		{
			//LOGI("[Audio] Stopping simulation | active instances: %zu", m_active.size());

			// Stop all tracked channels explicitly
			for (auto& inst : m_active)
			{
				if (inst.channel)
				{
					//LOGI("[Audio] Stopping channel | Slot: %s", inst.slotId.c_str());
					inst.channel->stop();
					inst.channel = nullptr;
				}
			}
			m_active.clear();
			m_commands.clear();
			m_playOnStartDone.clear();

#ifndef DISABLE_FMOD
			// Stop at group level as catch-all for any untracked channels
			if (m_groupAmbienceOutdoor) m_groupAmbienceOutdoor->stop();
			if (m_groupAmbienceIndoor)  m_groupAmbienceIndoor->stop();
			if (m_groupAmbience)        m_groupAmbience->stop();
			if (m_groupBGM)             m_groupBGM->stop();
			if (m_groupSFX)             m_groupSFX->stop();
			if (m_groupUI)              m_groupUI->stop();
			if (m_groupMaster)          m_groupMaster->stop();

			// Reset ambience state for clean re-entry
			if (m_groupAmbienceOutdoor) m_groupAmbienceOutdoor->setVolume(1.0f);
			if (m_groupAmbienceIndoor)  m_groupAmbienceIndoor->setVolume(0.0f);
			if (m_ambienceLowpass)
			{
				m_ambienceLowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 20000.0f);
				m_ambienceLowpass->setBypass(true);
			}
			m_indoorFactor = 0.0f;
#endif
		}
	}

	void AudioSystem::SetIndoorFactor(float factor)
	{
		m_indoorFactor = std::clamp(factor, 0.0f, 1.0f);

#ifndef DISABLE_FMOD
		if (!m_ambienceLowpass) return;

		// 0.0 outdoor = clear, 1.0 indoor = muffled
		const float cutoffOutdoor = 20000.0f;
		const float cutoffIndoor = 1200.0f;
		const float cutoff = cutoffOutdoor + (cutoffIndoor - cutoffOutdoor) * m_indoorFactor;

		m_ambienceLowpass->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff);
		m_ambienceLowpass->setBypass(m_indoorFactor < 0.01f);

		const float minOutdoorVolume = 1.0f;
		const float outdoorVol = 1.0f - (m_indoorFactor * (1.0f - minOutdoorVolume));

		if (m_groupAmbienceOutdoor) m_groupAmbienceOutdoor->setVolume(outdoorVol);

		// DEBUG
		float confirmedVol = -1.0f;
		if (m_groupAmbienceOutdoor) m_groupAmbienceOutdoor->getVolume(&confirmedVol);
		//LOGI("[Audio] SetIndoorFactor=%.2f | OutdoorGroupVol set =%.2f | confirmed=%.2f", m_indoorFactor, outdoorVol, confirmedVol);
#endif
	}



} // namespace EngineFramework
