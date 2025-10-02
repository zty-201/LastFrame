#include "pch.hpp"
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef LEVEL_EDITOR_LFEATURE_HPP
#define LEVEL_EDITOR_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"

#include "System_LevelEditor.hpp"
#include "Level_Editor.hpp"


namespace EngineFramework
{
	struct iEditorView; //forward declaration

	class LFeature
	{
	protected:
		iEditorView* editor_ptr = nullptr;// back-pointer to the limited variables
	public:

		bool is_enabled = false;
		virtual ~LFeature() = default;
		virtual void Initialize() = 0;
		virtual void Run() = 0;
		virtual void Close() = 0;
		void set_Pointer(iEditorView* ptr) { editor_ptr = ptr; }

	};



}




#endif
#endif
