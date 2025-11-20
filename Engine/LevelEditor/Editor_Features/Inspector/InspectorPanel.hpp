/*****************************************************************/
/*!
\file   InspectorPanel.hpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the declaration of the functions that are needed to
set up the window and fetches the components meta references.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/
#pragma once
#include <entt/entt.hpp>

namespace EditorFeatures {
	//Draws the inspector panel. is_display_entity is used to decide if the entity or the model properties should be displayed depending on which was selected the latest.
	void DrawInspector(entt::registry& reg, entt::entity entity_selected, xresource::full_guid model_selected, bool is_display_entity);
}
