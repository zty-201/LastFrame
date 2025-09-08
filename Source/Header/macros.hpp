/******************************************************************************/
/*!
\file		macros.hpp
\author 	Joel Lee Jie
\par    	joeljie.lee\@digipen.edu
\brief  	Contains frequently used macros, preprocessor definitions and constants.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
*/
/******************************************************************************/
#ifndef MACROS_HPP
#define MACROS_HPP

//Used to denote a variable is unused.
#define UNREFERENCED_PARAMETER(P) (P)
//Defines max delta time so the game doesn't have too big of a delta time value, like if the user shifts the window. Arbitrary value.
#define MAX_DELTA_TIME 0.1f
#define PI 3.14159265358979323846f

#endif