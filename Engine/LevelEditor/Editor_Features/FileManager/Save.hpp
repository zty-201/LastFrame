#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#ifndef SAVE_LFEATURE_HPP
#define SAVE_LFEATURE_HPP
#include <unordered_map>
#include <string>
#include <sstream>
#include "ISystem.hpp"
#include "entt/entity/registry.hpp"

#include "System_LevelEditor.hpp"
#include "LFeature.hpp"


namespace EngineFramework
{
    struct iEditorView; //forward declaration

    class Save : public LFeature
    {
    public:

        Save();
        ~Save() override;

        void Initialize() override;
        void Run() override;
        void Close() override;


    };



}




#endif
#endif
