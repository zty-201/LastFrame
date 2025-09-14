#ifdef _DEBUG
#ifndef OBJECTMANAGER_LFEATURE_HPP
#define OBJECTMANAGER_LFEATURE_HPP
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

    class ObjectManager : public LFeature
    {
    public:

        ObjectManager();
        ~ObjectManager() override;

        void Initialize() override;
        void Run() override;
        void Close() override;

    };

}




#endif
#endif
