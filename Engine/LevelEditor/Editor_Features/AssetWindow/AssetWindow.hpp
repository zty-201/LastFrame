#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef ASSETWINDOW_LFEATURE_HPP
#define ASSETWINDOW_LFEATURE_HPP

#include "ISystem.hpp"
#include "entt/entity/registry.hpp"
#include "System_LevelEditor.hpp"
#include "LFeature/LFeature.hpp"

namespace EngineFramework
{
	struct iEditorview; //Forward declaration

	class AssetWindow : public LFeature
	{
	public:
		AssetWindow();
		~AssetWindow() override;

		void Initialize() override;
		void Run() override;
		void Close() override;

		static AssetWindow& GetInstance();

		static bool feature_enabled;

	private:
		static inline AssetWindow* this_system{};
	};
}

#endif
#endif