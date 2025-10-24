/*
*****************************************************************/
/*!
\file   ScriptDebug.hpp
\author Teng Jun Rong Jerron
\par    t.junrongjerron\@digipen.edu
\date   October 2025
\brief
This file contains the declaration of the functions that are needed for
debugging if Scripts are working correctly.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#pragma once

#if defined(ENGINE_ENABLE_LUA) && defined(DEBUG)

namespace Engine {
    class ScriptDebug {
    public:
        static void Initialize();
        static void Update(float dt);
        static void Shutdown();
    };
}

#endif // ENGINE_ENABLE_LUA && DEBUG