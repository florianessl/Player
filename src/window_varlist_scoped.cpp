/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include <sstream>
#include <iomanip>
#include "window_varlist_scoped.h"
#include "game_switches.h"
#include "game_variables.h"
#include "bitmap.h"
#include <lcf/data.h>
#include "output.h"
#include <lcf/reader_util.h>
#include "game_map.h"

Window_VarListScoped::Window_VarListScoped() :
Window_Selectable(0, 32, 224, 176) {
	menu_item_height = 32;
	item_max = 8;
	column_max = 2;
	for (int i = 0; i < item_max; i++)
		items.push_back("");
	SetContents(Bitmap::Create(this->width - 16, 5 * menu_item_height));
}

Window_VarListScoped::~Window_VarListScoped() {

}

void Window_VarListScoped::Refresh() {
	contents->Clear();
	for (int i = 0; i < 4; i++) {
		DrawItemValue(i);
	}
	if (scope_map_id > 0) {
		switch (mode) {
			case eMapSwitch:
			case eMapVariable:
				contents->TextDraw(4, 2, Font::ColorCritical, fmt::format("<Map{:04d}: {}>", scope_map_id, Game_Map::GetMapName(scope_map_id)), Text::AlignLeft);
				if ((mode == eMapSwitch && !Main_Data::game_switches->scoped_map.IsStorageInitialized(scope_map_id))
					|| (mode == eMapVariable && !Main_Data::game_variables->scoped_map.IsStorageInitialized(scope_map_id))) {
					contents->TextDraw(GetWidth() - 16, 2, Font::ColorDisabled, "(empty)", Text::AlignRight);
				}
				contents->TextDraw(0, 18, Font::ColorCritical, "r:", Text::AlignLeft);
				contents->TextDraw(13, 18, Font::ColorDefault, "readonly", Text::AlignLeft);
				contents->TextDraw(67, 18, Font::ColorCritical, "a:", Text::AlignLeft);
				contents->TextDraw(81, 18, Font::ColorDefault, "auto-reset", Text::AlignLeft);
				contents->TextDraw(144, 18, Font::ColorCritical, "i", Text::AlignLeft);
				contents->TextDraw(148, 18, Font::ColorCritical, ":", Text::AlignLeft);
				contents->TextDraw(154, 18, Font::ColorDefault, "inherited", Text::AlignLeft);
				break;
			case eMapEventSwitch:
			case eMapEventVariable:
				contents->TextDraw(4, 2, Font::ColorCritical, fmt::format("<Map{:04d}: {}, EV{:04d}>", scope_map_id, Game_Map::GetMapName(scope_map_id), scope_evt_id), Text::AlignLeft);
				if ((mode == eMapEventSwitch && !Main_Data::game_switches->scoped_mapevent.IsStorageInitialized(scope_map_id, scope_evt_id))
					|| (mode == eMapEventVariable && !Main_Data::game_variables->scoped_mapevent.IsStorageInitialized(scope_map_id, scope_evt_id))) {
					contents->TextDraw(GetWidth() - 16, 18, Font::ColorDisabled, "(empty)", Text::AlignRight);
				}
				contents->TextDraw(0, 18, Font::ColorCritical, "r:", Text::AlignLeft);
				contents->TextDraw(14, 18, Font::ColorDefault, "readonly", Text::AlignLeft);
				contents->TextDraw(68, 18, Font::ColorCritical, "a:", Text::AlignLeft);
				contents->TextDraw(82, 18, Font::ColorDefault, "auto-reset", Text::AlignLeft);
				break;
		}
	}
}

void Window_VarListScoped::DrawItemValue(int index){
	if (!DataIsValid(first_var + index)) {
		return;
	}
	int value = 0;
	bool defined = false, inherited = false, readonly = false, default_val_defined = false, auto_reset = false, defined_in_tree = false;
	bool default_value_b = false;
	int default_value_i = 0;
	static std::stringstream ss;

	auto get_parent_map_id = [](int map_id) {
		auto map_info = Game_Map::GetMapInfo(map_id);
		return map_info.parent_map;
	};

	switch (mode) {
		case eMapSwitch:
			value = Main_Data::game_switches->Get<eDataScope_Map>(first_var + index, scope_map_id);
			defined = Main_Data::game_switches->scoped_map.IsDefined(first_var + index, scope_map_id);
			inherited = Main_Data::game_switches->scoped_map.IsInheritedValue(first_var + index, scope_map_id);
			readonly = Main_Data::game_switches->scoped_map.IsReadOnly(first_var + index, scope_map_id);
			default_val_defined = Main_Data::game_switches->scoped_map.IsDefaultValueDefined(first_var + index, scope_map_id);
			auto_reset = Main_Data::game_switches->scoped_map.IsAutoReset(first_var + index, scope_map_id);
			if (default_val_defined) {
				default_value_b = Main_Data::game_switches->scoped_map.GetDefaultValue(first_var + index);
			}
			if (!defined && inherited) {
				game_bool value_b = false;
				defined_in_tree = Main_Data::game_switches->scoped_map.GetInherited(first_var + index, scope_map_id, get_parent_map_id, value_b);
				value = value_b;
			}
			break;
		case eMapVariable:
			value = Main_Data::game_variables->Get<eDataScope_Map>(first_var + index, scope_map_id);
			defined = Main_Data::game_variables->scoped_map.IsDefined(first_var + index, scope_map_id);
			inherited = Main_Data::game_variables->scoped_map.IsInheritedValue(first_var + index, scope_map_id);
			readonly = Main_Data::game_variables->scoped_map.IsReadOnly(first_var + index, scope_map_id);
			default_val_defined = Main_Data::game_variables->scoped_map.IsDefaultValueDefined(first_var + index, scope_map_id);
			auto_reset = Main_Data::game_variables->scoped_map.IsAutoReset(first_var + index, scope_map_id);
			if (default_val_defined) {
				default_value_i = Main_Data::game_variables->scoped_map.GetDefaultValue(first_var + index);
			}
			if (!defined && inherited) {
				defined_in_tree = Main_Data::game_variables->scoped_map.GetInherited(first_var + index, scope_map_id, get_parent_map_id, value);
			}
			break;
		case eMapEventSwitch:
			value = Main_Data::game_switches->Get<eDataScope_MapEvent>(first_var + index, scope_map_id, scope_evt_id);
			defined = Main_Data::game_switches->scoped_mapevent.IsDefined(first_var + index, scope_map_id, scope_evt_id);
			readonly = Main_Data::game_switches->scoped_mapevent.IsReadOnly(first_var + index, scope_map_id, scope_evt_id);
			default_val_defined = Main_Data::game_switches->scoped_mapevent.IsDefaultValueDefined(first_var + index, scope_map_id, scope_evt_id);
			auto_reset = Main_Data::game_switches->scoped_mapevent.IsAutoReset(first_var + index, scope_map_id, scope_evt_id);
			if (default_val_defined) {
				default_value_b = Main_Data::game_switches->scoped_mapevent.GetDefaultValue(first_var + index);
			}
			break;
		case eMapEventVariable:
			value = Main_Data::game_variables->Get<eDataScope_MapEvent>(first_var + index, scope_map_id, scope_evt_id);
			defined = Main_Data::game_variables->scoped_mapevent.IsDefined(first_var + index, scope_map_id, scope_evt_id);
			readonly = Main_Data::game_variables->scoped_mapevent.IsReadOnly(first_var + index, scope_map_id, scope_evt_id);
			default_val_defined = Main_Data::game_variables->scoped_mapevent.IsDefaultValueDefined(first_var + index, scope_map_id, scope_evt_id);
			auto_reset = Main_Data::game_variables->scoped_mapevent.IsAutoReset(first_var + index, scope_map_id, scope_evt_id);
			if (default_val_defined) {
				default_value_i = Main_Data::game_variables->scoped_mapevent.GetDefaultValue(first_var + index);
			}
			break;
		default:
		return;
	}
	ss.str("");
	bool b_flags = false;

	if (readonly) {
		ss << "r";
		b_flags = true;
	}
	if (auto_reset) {
		if (b_flags)
			ss << ",";
		ss << "a";
		b_flags = true;
	}
	if (inherited) {
		if (b_flags)
			ss << ",";
		ss << "i";
		b_flags = true;
	}

	int y = menu_item_height * (index + 1) + 2;

	contents->ClearRect(Rect(0, y, contents->GetWidth() - 0, menu_item_height));
	contents->TextDraw(0, y, Font::ColorDefault, items[index * 2]);

	int x_val = GetWidth() - 16;

	if (!defined && !defined_in_tree) {
		contents->TextDraw(x_val, y, Font::ColorDisabled, "undefined", Text::AlignRight);
	} else {
		if (inherited && defined_in_tree) {
			contents->TextDraw(x_val, y, Font::ColorHeal, "*", Text::AlignRight);
			x_val -= 8;
		}
		if (mode == eMapSwitch || mode == eMapEventSwitch) {
			contents->TextDraw(x_val, y, (inherited && defined_in_tree) ? Font::ColorDisabled : Font::ColorDefault, value ? "[ON]" : "[OFF]", Text::AlignRight);
		} else {
			contents->TextDraw(x_val, y, (inherited&& defined_in_tree) ? Font::ColorDisabled : Font::ColorDefault, std::to_string(value), Text::AlignRight);
		}
	}
	if (b_flags) {
		contents->TextDraw(8, y + 16, Font::ColorCritical, fmt::format("({})",  ss.str()), Text::AlignLeft);
	}
	if (default_val_defined) {
		if (mode == eMapSwitch || mode == eMapEventSwitch) {
			contents->TextDraw(60, y + 16, Font::ColorDefault, fmt::format("default: {}", default_value_b ? "[ON]" : "[OFF]"), Text::AlignLeft);
		} else {
			contents->TextDraw(60, y + 16, Font::ColorDefault, fmt::format("default: {}", std::to_string(default_value_i)), Text::AlignLeft);
		}
	}
	if (inherited && defined_in_tree) {
		static std::stringstream ss;

		int parent_id = scope_map_id;
		int max_digits = 3;
		while ((parent_id = get_parent_map_id(parent_id)) > 0) {
			if (parent_id > 999)
				max_digits = 4;
		}
	    parent_id = scope_map_id;
		ss.str("");
		ss << "* inh.: .";
		while ((parent_id = get_parent_map_id(parent_id)) > 0) {
			ss << " > " << std::setfill('0') << std::setw(max_digits) << (Game_Map::GetMapInfo(parent_id).ID);
		}
		contents->TextDraw(x_val + 8, y + 16, Font::ColorHeal, ss.str(), Text::AlignRight);
		x_val -= 8;
	}
}

void Window_VarListScoped::UpdateList(int first_value){
	static std::stringstream ss;
	first_var = first_value;
	for (int i = 0; i < 4; i++){
		if (!DataIsValid(first_var + i)) {
			continue;
		}
		ss.str("");
		switch (mode) {
			case eMapSwitch:
			case eMapVariable:
				ss << std::setfill('0') << std::setw(3) << (first_value + i) << ": ";
				break;
			case eMapEventSwitch:
			case eMapEventVariable:
				ss << std::setfill('0') << std::setw(2) << (first_value + i) << ": ";
				break;
			default:
				ss << std::setfill('0') << std::setw(4) << (first_value + i) << ": ";
				break;
		}
		switch (mode) {
			case eMapSwitch:
				ss << Main_Data::game_switches->GetName(first_value + i, eDataScope_Map);
				break;
			case eMapVariable:
				ss << Main_Data::game_variables->GetName(first_value + i, eDataScope_Map);
				break;
			case eMapEventSwitch:
				ss << Main_Data::game_switches->GetName(first_value + i, eDataScope_MapEvent);
				break;
			case eMapEventVariable:
				ss << Main_Data::game_variables->GetName(first_value + i, eDataScope_MapEvent);
				break;
			default:
				break;
		}
		if (i * 2 < items.size()) {
			items[i * 2] = ss.str();
		}
	}
	Refresh();
}

void Window_VarListScoped::UpdateCursorRect() {
	Window_Selectable::UpdateCursorRect();
	if (index % 2 == 0) {
		cursor_rect = { cursor_rect.x, cursor_rect.y + 31, (cursor_rect.width * 2) - 56, cursor_rect.height };
	} else {
		cursor_rect = { cursor_rect.x + cursor_rect.width - 64, cursor_rect.y + 31, 64, cursor_rect.height };
	}
}

void Window_VarListScoped::SetMode(Mode mode) {
	this->mode = mode;
	SetVisible((mode != eNone));
	Refresh();
}

bool Window_VarListScoped::DataIsValid(int range_index) {
	switch (mode) {
		case eMapSwitch:
			if (scope_map_id == 0)
				return false;
			return Main_Data::game_switches->IsValid<eDataScope_Map>(range_index);
		case eMapVariable:
			if (scope_map_id == 0)
				return false;
			return Main_Data::game_variables->IsValid<eDataScope_Map>(range_index);
		case eMapEventSwitch:
			if (scope_map_id == 0 || scope_evt_id == 0)
				return false;
			return Main_Data::game_switches->IsValid<eDataScope_MapEvent>(range_index);
		case eMapEventVariable:
			if (scope_map_id == 0 || scope_evt_id == 0)
				return false;
			return Main_Data::game_variables->IsValid<eDataScope_MapEvent>(range_index);
		default:
			break;
	}
	return false;
}

void Window_VarListScoped::SetScope(int map_id, int evt_id) {
	scope_map_id = map_id;
	scope_evt_id = evt_id;
}
