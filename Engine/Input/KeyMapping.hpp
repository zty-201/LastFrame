/*
*****************************************************************/
/*!
\file   LeyMapping.hpp
\author Tan Yee Tong
\par    yeetong.t\@digipen.edu
\date   October 2025
\brief
This file contains the declarations of the functions that are needed for
handling the key_mapping inputs in the system. 

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#if defined(DEBUG) && defined(PLATFORM_WINDOWS)

#ifndef KEYMAPPING_H
#define KEYMAPPING_H

#include <unordered_map>

#ifdef PLATFORM_WINDOWS
    #include "GLFW/glfw3.h"
#endif

#if defined(DEBUG)
    #include "imgui.h"
    #include "imgui_impl_glfw.h"
    #include "imgui_impl_opengl3.h"
#endif 

#ifdef PLATFORM_ANDROID
    #include <android/native_activity.h>
    #include <android/input.h>
#endif

namespace EngineFramework
{

    struct IMGUI_GLFW_KEYPAIR { 
        ImGuiKey imgui; 
        int glfw; 
    };

    /*
        Why are we doing this? Imgui Key mapping is not publicly avaiable to us. It's in imgui backend
        which is not accessible for us. We have to reverse the mapping, vice versa, so we are
        doing it.

        NOTE: Remember this is only available on debug where level editor is in use.
    
    */

    static constexpr IMGUI_GLFW_KEYPAIR All_KeyPair[] = {
        
        { ImGuiKey_A, GLFW_KEY_A }, { ImGuiKey_B, GLFW_KEY_B }, { ImGuiKey_C, GLFW_KEY_C },
        { ImGuiKey_D, GLFW_KEY_D }, { ImGuiKey_E, GLFW_KEY_E }, { ImGuiKey_F, GLFW_KEY_F },
        { ImGuiKey_G, GLFW_KEY_G }, { ImGuiKey_H, GLFW_KEY_H }, { ImGuiKey_I, GLFW_KEY_I },
        { ImGuiKey_J, GLFW_KEY_J }, { ImGuiKey_K, GLFW_KEY_K }, { ImGuiKey_L, GLFW_KEY_L },
        { ImGuiKey_M, GLFW_KEY_M }, { ImGuiKey_N, GLFW_KEY_N }, { ImGuiKey_O, GLFW_KEY_O },
        { ImGuiKey_P, GLFW_KEY_P }, { ImGuiKey_Q, GLFW_KEY_Q }, { ImGuiKey_R, GLFW_KEY_R },
        { ImGuiKey_S, GLFW_KEY_S }, { ImGuiKey_T, GLFW_KEY_T }, { ImGuiKey_U, GLFW_KEY_U },
        { ImGuiKey_V, GLFW_KEY_V }, { ImGuiKey_W, GLFW_KEY_W }, { ImGuiKey_X, GLFW_KEY_X },
        { ImGuiKey_Y, GLFW_KEY_Y }, { ImGuiKey_Z, GLFW_KEY_Z },

        
        { ImGuiKey_0, GLFW_KEY_0 }, { ImGuiKey_1, GLFW_KEY_1 }, { ImGuiKey_2, GLFW_KEY_2 },
        { ImGuiKey_3, GLFW_KEY_3 }, { ImGuiKey_4, GLFW_KEY_4 }, { ImGuiKey_5, GLFW_KEY_5 },
        { ImGuiKey_6, GLFW_KEY_6 }, { ImGuiKey_7, GLFW_KEY_7 }, { ImGuiKey_8, GLFW_KEY_8 },
        { ImGuiKey_9, GLFW_KEY_9 },

        
        { ImGuiKey_F1, GLFW_KEY_F1 }, { ImGuiKey_F2, GLFW_KEY_F2 }, { ImGuiKey_F3, GLFW_KEY_F3 },
        { ImGuiKey_F4, GLFW_KEY_F4 }, { ImGuiKey_F5, GLFW_KEY_F5 }, { ImGuiKey_F6, GLFW_KEY_F6 },
        { ImGuiKey_F7, GLFW_KEY_F7 }, { ImGuiKey_F8, GLFW_KEY_F8 }, { ImGuiKey_F9, GLFW_KEY_F9 },
        { ImGuiKey_F10, GLFW_KEY_F10 }, { ImGuiKey_F11, GLFW_KEY_F11 }, { ImGuiKey_F12, GLFW_KEY_F12 },

        
        { ImGuiKey_Space, GLFW_KEY_SPACE },
        { ImGuiKey_Enter, GLFW_KEY_ENTER },
        { ImGuiKey_Escape, GLFW_KEY_ESCAPE },
        { ImGuiKey_Backspace, GLFW_KEY_BACKSPACE },
        { ImGuiKey_Tab, GLFW_KEY_TAB },
        { ImGuiKey_LeftArrow, GLFW_KEY_LEFT },
        { ImGuiKey_RightArrow, GLFW_KEY_RIGHT },
        { ImGuiKey_UpArrow, GLFW_KEY_UP },
        { ImGuiKey_DownArrow, GLFW_KEY_DOWN },
        { ImGuiKey_PageUp, GLFW_KEY_PAGE_UP },
        { ImGuiKey_PageDown, GLFW_KEY_PAGE_DOWN },
        { ImGuiKey_Home, GLFW_KEY_HOME },
        { ImGuiKey_End, GLFW_KEY_END },
        { ImGuiKey_Insert, GLFW_KEY_INSERT },
        { ImGuiKey_Delete, GLFW_KEY_DELETE },

        
        { ImGuiKey_LeftShift, GLFW_KEY_LEFT_SHIFT },
        { ImGuiKey_RightShift, GLFW_KEY_RIGHT_SHIFT },
        { ImGuiKey_LeftCtrl, GLFW_KEY_LEFT_CONTROL },
        { ImGuiKey_RightCtrl, GLFW_KEY_RIGHT_CONTROL },
        { ImGuiKey_LeftAlt, GLFW_KEY_LEFT_ALT },
        { ImGuiKey_RightAlt, GLFW_KEY_RIGHT_ALT },
        { ImGuiKey_LeftSuper, GLFW_KEY_LEFT_SUPER },
        { ImGuiKey_RightSuper, GLFW_KEY_RIGHT_SUPER },

        
        { ImGuiKey_Keypad0, GLFW_KEY_KP_0 },
        { ImGuiKey_Keypad1, GLFW_KEY_KP_1 },
        { ImGuiKey_Keypad2, GLFW_KEY_KP_2 },
        { ImGuiKey_Keypad3, GLFW_KEY_KP_3 },
        { ImGuiKey_Keypad4, GLFW_KEY_KP_4 },
        { ImGuiKey_Keypad5, GLFW_KEY_KP_5 },
        { ImGuiKey_Keypad6, GLFW_KEY_KP_6 },
        { ImGuiKey_Keypad7, GLFW_KEY_KP_7 },
        { ImGuiKey_Keypad8, GLFW_KEY_KP_8 },
        { ImGuiKey_Keypad9, GLFW_KEY_KP_9 },
        { ImGuiKey_KeypadDecimal, GLFW_KEY_KP_DECIMAL },
        { ImGuiKey_KeypadDivide, GLFW_KEY_KP_DIVIDE },
        { ImGuiKey_KeypadMultiply, GLFW_KEY_KP_MULTIPLY },
        { ImGuiKey_KeypadSubtract, GLFW_KEY_KP_SUBTRACT },
        { ImGuiKey_KeypadAdd, GLFW_KEY_KP_ADD },
        { ImGuiKey_KeypadEnter, GLFW_KEY_KP_ENTER },
        { ImGuiKey_KeypadEqual, GLFW_KEY_KP_EQUAL },
    }; 

    inline int Get_GLFW_Key(ImGuiKey key) {

       for (IMGUI_GLFW_KEYPAIR  pair : All_KeyPair) {

           if (pair.imgui == key) {
               return pair.glfw;
           }
       }
       
       return GLFW_KEY_UNKNOWN;
    }
    

    inline ImGuiKey Get_IMGUI_Key(int key) {

        for (IMGUI_GLFW_KEYPAIR pair : All_KeyPair) {

            if (pair.glfw == key) {
                return pair.imgui;
            }
        }
        return ImGuiKey_None;
    }
   

}

#endif
#endif