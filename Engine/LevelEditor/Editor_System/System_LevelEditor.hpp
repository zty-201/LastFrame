/*
*****************************************************************/
/*!
\file   System_LevelEditor.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   September 2025
\brief
This file is the levele editor system
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef SYSTEM_LEVEL_EDITOR_HPP
#define SYSTEM_LEVEL_EDITOR_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "Level_Editor.hpp"

namespace EngineFramework
{
	

	/*

	*/
	class LevelEditorSystem : public ISystem
	{
	public:
		LevelEditorSystem();

	private:
		void Initialize() override;
		void Update(float delta_time) override;

		//Keeps track of the one and only instance of this system, created by core engine.
		static inline LevelEditorSystem* this_system{};

		std::unique_ptr<LevelEditor> application;
	public:

		static bool Get_Enable();
		
		struct ProfilerSectionSample {
			double ms = 0.0;
			int calls = 0;
			bool valid = false;
		};

		struct ProfilingData {
			ProfilerSectionSample update;
			ProfilerSectionSample run;
		};

		static const ProfilingData& GetProfilingData();

	};


}




#endif
#endif
