/*****************************************************************/
/*!
\file   MetaInspector.cpp
\author Neo HuiZong
\par    neo.h\@digipen.edu
\date   October 2025
\brief
This file contains the declaration of the functions that are needed to
draw the controls and pushes the changes straight into live components
of the selected entity.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* *******************************************************************/

#pragma once
#include <entt/entt.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "xresource_guid.h"

// Ensure the tag types backing xresource GUIDs are complete when this header is included without pch.
namespace xresource {
	struct rsc_instance_guid_tag {};
	struct rsc_instance_guid_large_tag {};
	struct rsc_type_guid_tag {};
}

namespace EditorFeatures {

	using AssetDisplayFormatter = std::function<std::string(const xresource::full_guid& guid, std::string display_name, std::string_view suffix)>;

	struct AssetPickerOptions {
		xresource::type_guid type_guid;
		std::string_view suffix;
		std::vector<std::pair<xresource::instance_guid, std::string>> entries;
	};

	enum class AssetPickerKind : std::uint32_t {
		Texture = 0,
		Audio = 1,
		Script = 2

	}; // Add on further if any - idk what else to add

	struct AssetPickerDescriptor {
		std::string_view none_label;
		std::vector<AssetPickerOptions> sources;
		AssetDisplayFormatter formatter;
	};

	struct AssetPickerTag {
		AssetPickerKind kind;
	};

	void DrawMetaObject(entt::meta_type& mt, entt::meta_any& obj);
}

