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

#include "game_interpreter_shared.h"
#include "game_actors.h"
#include "game_enemyparty.h"
#include "game_ineluki.h"
#include "game_map.h"
#include "game_party.h"
#include "game_player.h"
#include "game_switches.h"
#include "game_system.h"
#include "maniac_patch.h"
#include "main_data.h"
#include "output.h"
#include "player.h"
#include "rand.h"
#include "util_macro.h"
#include "utils.h"
#include "audio.h"
#include "baseui.h"
#include <cmath>
#include <cstdint>
#include <lcf/rpg/savepartylocation.h>

using Main_Data::game_switches, Main_Data::game_variables, Main_Data::game_strings;

template<bool validate_patches, bool support_range_indirect, bool support_expressions, bool support_bitmask, bool support_scopes, bool support_named>
inline bool Game_Interpreter_Shared::DecodeTargetEvaluationMode(lcf::rpg::EventCommand const& com, int& id_0, int& id_1, Game_BaseInterpreterContext const& interpreter) {
	int mode = com.parameters[0];

	if constexpr (support_bitmask) {
		mode = com.parameters[0] & 15;
	}

	switch (mode) {
		case TargetEvalMode::eTargetEval_Single:
			id_0 = com.parameters[1];
			id_1 = id_0;
			break;
		case TargetEvalMode::eTargetEval_Range:
			id_0 = com.parameters[1];
			id_1 = com.parameters[2];
			break;
		case TargetEvalMode::eTargetEval_IndirectSingle:
			id_0 = game_variables->Get(com.parameters[1]);
			id_1 = id_0;
			break;
		case TargetEvalMode::eTargetEval_IndirectRange:
			if constexpr (!support_range_indirect) {
				return false;
			}
			if constexpr (validate_patches) {
				if (!Player::IsPatchManiac()) {
					return false;
				}
			}
			id_0 = game_variables->Get(com.parameters[1]);
			id_1 = game_variables->Get(com.parameters[2]);
			break;
		case TargetEvalMode::eTargetEval_Expression:
			if constexpr (!support_expressions) {
				return false;
			}
			if constexpr (validate_patches) {
				if (!Player::IsPatchManiac()) {
					return false;
				}
			}
			{
				// Expression (Maniac)
				int idx = com.parameters[1];
				id_0 = ManiacPatch::ParseExpression(MakeSpan(com.parameters).subspan(idx + 1, com.parameters[idx]), interpreter);
				id_1 = id_0;
				return true;
			}
			break;
		case TargetEvalMode::eTargetEval_CodedInParam2:
		{
			int modeExt = com.parameters[2] & 15;

			if constexpr (!support_named) {
				return false;
			}
			if (modeExt == TargetEvalExtMode::eTargetEvalExt_Named || modeExt == TargetEvalExtMode::eTargetEvalExt_NamedString || modeExt == TargetEvalExtMode::eTargetEvalExt_NamedStringIndirect) {
				// Named Variables
				if constexpr (validate_patches) {
					if (!Player::HasEasyRpgExtensions()) {
						return false;
					}
				}
				int pos = 0;
				StringView var_name;

				switch (mode) {
					case eTargetEvalExt_Named:
						var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, com.parameters[1], &pos, *game_variables);
						break;
					case eTargetEvalExt_NamedString:
						var_name = game_strings->GetWithMode(com.string, Game_Strings::StringEvalMode::eStringEval_Direct, com.parameters[1], *game_variables);
						break;
					case eTargetEvalExt_NamedStringIndirect:
						var_name = game_strings->GetWithMode(com.string, Game_Strings::StringEvalMode::eStringEval_Indirect, com.parameters[1], *game_variables);
						break;
				}

				if (!GetVariableIdByName(var_name, id_0))
					return false;
				id_1 = id_0;

				return true;
			} else {
				Output::Warning("TargetEval: Unsupported mode extension {}", modeExt);

				return false;
			}
			break;
		}
		default:
		{
			if constexpr (support_scopes) {
				int scoped_var_id, map_id, evt_id;

				switch (mode) {
					case TargetEvalMode::eTargetEval_FrameScope_IndirectSingle:
						id_0 = game_variables->Get<DataScopeType::eDataScope_Frame>(com.parameters[1], &interpreter.GetFrame());
						id_1 = id_0;
						break;
					case TargetEvalMode::eTargetEval_FrameScope_IndirectRange:
						id_0 = game_variables->Get<DataScopeType::eDataScope_Frame>(com.parameters[1], &interpreter.GetFrame());
						id_1 = game_variables->Get<DataScopeType::eDataScope_Frame>(com.parameters[2], &interpreter.GetFrame());
						break;
					case TargetEvalMode::eTargetEval_MapScope_IndirectSingle:
					case TargetEvalMode::eTargetEval_MapScope_IndirectRange:
						if (!UnpackMapScopedVarId(com.parameters[1], scoped_var_id, map_id, com, interpreter)) {
							return false;
						}
						id_0 = EvaluateMapTreeVariable(0, scoped_var_id, map_id);
						if (mode == TargetEvalMode::eTargetEval_MapScope_IndirectRange) {
							if (!UnpackMapScopedVarId(com.parameters[2], scoped_var_id, map_id, com, interpreter)) {
								return false;
							}
							id_1 = EvaluateMapTreeVariable(0, scoped_var_id, map_id);
						} else {
							id_1 = id_0;
						}
						break;
					case TargetEvalMode::eTargetEval_MapEventScope_IndirectSingle:
					case TargetEvalMode::eTargetEval_MapEventScope_IndirectRange:
						if (!UnpackMapEventScopedVarId(com.parameters[1], scoped_var_id, map_id, evt_id, com, interpreter)) {
							return false;
						}
						id_0 = game_variables->Get<DataScopeType::eDataScope_MapEvent>(scoped_var_id, map_id, evt_id);
						if (mode == TargetEvalMode::eTargetEval_MapEventScope_IndirectRange) {
							if (!UnpackMapEventScopedVarId(com.parameters[2], scoped_var_id, map_id, evt_id, com, interpreter)) {
								return false;
							}
							id_1 = game_variables->Get<DataScopeType::eDataScope_MapEvent>(scoped_var_id, map_id, evt_id);
						} else {
							id_1 = id_0;
						}
						break;
				}
			}
		}
	}

	if constexpr (validate_patches) {
		if (Player::IsPatchManiac() && id_1 < id_0) {
			// Vanilla does not support end..start, Maniac does
			std::swap(id_0, id_1);
		}
	} else {
		if (id_1 < id_0) {
			std::swap(id_0, id_1);
		}
	}

	return true;
}

template<bool validate_patches, bool support_indirect_and_switch, bool support_scopes, bool support_named>
int Game_Interpreter_Shared::ValueOrVariable(int mode, int val, Game_BaseInterpreterContext const& interpreter) {
	if (mode == ValueEvalMode::eValueEval_Constant) {
		return val;
	} else if (mode == ValueEvalMode::eValueEval_Variable) {
		return game_variables->Get(val);
	} else {
		if constexpr (support_indirect_and_switch) {
			if constexpr (validate_patches) {
				if (!Player::IsPatchManiac())
					return -1;
			}
			// Maniac Patch does not implement all modes for all commands
			// For simplicity it is enabled for all here
			if (mode == ValueEvalMode::eValueEval_VariableIndirect) {
				// Variable indirect
				return game_variables->GetIndirect(val);
			} else if (mode == ValueEvalMode::eValueEval_Switch) {
				// Switch (F = 0, T = 1)
				return game_switches->GetInt(val);
			} else if (mode == ValueEvalMode::eValueEval_SwitchIndirect) {
				// Switch through Variable (F = 0, T = 1)
				return game_switches->GetInt(game_variables->Get(val));
			}
		}
		if constexpr (support_scopes) {
			if constexpr (validate_patches) {
				if (!Player::HasEasyRpgExtensions())
					return -1;
			}
			int scoped_var_id, map_id, evt_id;
			switch (mode) {
				case ValueEvalMode::eValueEval_FrameScopeVariable:
					if (UnpackFrameScopedVarId(val, scoped_var_id, {}, interpreter)) {
						return game_variables->Get<eDataScope_Frame>(val, &interpreter.GetFrame());
					}
					break;
				case ValueEvalMode::eValueEval_MapScopeVariable:
				case ValueEvalMode::eValueEval_MapScopeVariableIndirect:
					if (UnpackMapScopedVarId(val, scoped_var_id, map_id, {}, interpreter)) {
						int v = (mode == ValueEvalMode::eValueEval_MapScopeVariable) ? EvaluateMapTreeVariable(0, scoped_var_id, map_id) : EvaluateMapTreeVariable(1, scoped_var_id, map_id);
						return v != 0 ? v : Game_Map::GetMapId();
					}
					break;
				case ValueEvalMode::eValueEval_MapEventScopeVariable:
				case ValueEvalMode::eValueEval_MapEventScopeVariableIndirect:
					if (UnpackMapEventScopedVarId(val, scoped_var_id, map_id, evt_id, {}, interpreter)) {
						int v = (mode == ValueEvalMode::eValueEval_MapEventScopeVariable) ? game_variables->Get<DataScopeType::eDataScope_MapEvent>(scoped_var_id, map_id, evt_id) : game_variables->scopedGetIndirect<DataScopeType::eDataScope_MapEvent, DataScopeType::eDataScope_Global>(scoped_var_id, map_id, 0, evt_id, 0);
						return v != 0 && v != Game_Character::CharThisEvent ? v : interpreter.GetThisEventId();
					}
					break;
			}
		}
	}
	return -1;
}

template<bool validate_patches, bool support_indirect_and_switch, bool support_scopes, bool support_named>
int Game_Interpreter_Shared::ValueOrVariableBitfield(int mode, int shift, int val, Game_BaseInterpreterContext const& interpreter) {
	return ValueOrVariable<validate_patches, support_indirect_and_switch, support_scopes, support_named>((mode & (0xF << shift * 4)) >> shift * 4, val, interpreter);
}

template<bool validate_patches, bool support_indirect_and_switch, bool support_scopes, bool support_named>
int Game_Interpreter_Shared::ValueOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx, Game_BaseInterpreterContext const& interpreter) {
	assert(com.parameters.size() > val_idx);

	if (!Player::IsPatchManiac()) {
		return com.parameters[val_idx];
	}

	assert(mode_idx != val_idx);

	if (com.parameters.size() > std::max(mode_idx, val_idx)) {
		int mode = com.parameters[mode_idx];
		return ValueOrVariableBitfield<validate_patches, support_indirect_and_switch, support_scopes, support_named>(com.parameters[mode_idx], shift, com.parameters[val_idx], interpreter);
	}

	return com.parameters[val_idx];
}

int Game_Interpreter_Shared::scopedValueOrVariable(int mode, int val, int map_id, int event_id) {
	if (mode == ValueEvalMode::eValueEval_MapScopeVariable || mode == ValueEvalMode::eValueEval_MapScopeVariableIndirect) {
		if (mode == ValueEvalMode::eValueEval_MapScopeVariable)
			return EvaluateMapTreeVariable(0, val, map_id);
		else
			return EvaluateMapTreeVariable(1, val, map_id);
	} else if (mode == ValueEvalMode::eValueEval_MapEventScopeVariable || mode == ValueEvalMode::eValueEval_MapEventScopeVariableIndirect) {
		if (mode == ValueEvalMode::eValueEval_MapEventScopeVariable)
			return game_variables->Get<DataScopeType::eDataScope_MapEvent>(val, map_id, event_id);
		else
			return game_variables->scopedGetIndirect<DataScopeType::eDataScope_MapEvent, DataScopeType::eDataScope_Global>(val, map_id, 0, event_id, 0);
	}
	return -1;
}


StringView Game_Interpreter_Shared::CommandStringOrVariable(lcf::rpg::EventCommand const& com, int mode_idx, int val_idx) {
	if (!Player::IsPatchManiac()) {
		return com.string;
	}

	assert(mode_idx != val_idx);

	if (com.parameters.size() > std::max(mode_idx, val_idx)) {
		return game_strings->GetWithMode(ToString(com.string), com.parameters[mode_idx], com.parameters[val_idx], *game_variables);
	}

	return com.string;
}

StringView Game_Interpreter_Shared::CommandStringOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx) {
	if (!Player::IsPatchManiac()) {
		return com.string;
	}

	assert(mode_idx != val_idx);

	if (com.parameters.size() >= std::max(mode_idx, val_idx) + 1) {
		int mode = com.parameters[mode_idx];
		return game_strings->GetWithMode(ToString(com.string), (mode & (0xF << shift * 4)) >> shift * 4, com.parameters[val_idx], *game_variables);
	}

	return com.string;
}

game_bool Game_Interpreter_Shared::EvaluateMapTreeSwitch(int mode, int switch_id, int map_id) {

	auto get_parent_map_id = [](int map_id) {
		auto map_info = Game_Map::GetMapInfo(map_id);
		return map_info.parent_map;
	};

	if (mode == 1) {
		switch_id = game_variables->GetIndirect(switch_id);
	}

	game_bool value = false;
	bool is_defined = game_switches->scoped_map.GetInherited(switch_id, map_id, get_parent_map_id, value);
	if (!is_defined && game_switches->scoped_map.IsDefaultValueDefined(switch_id, map_id)) {
		value = game_switches->scoped_map.GetDefaultValue(switch_id);
	}
	return value;
}

int Game_Interpreter_Shared::EvaluateMapTreeVariable(int mode, int var_id, int map_id) {

	auto get_parent_map_id = [](int map_id) {
		auto map_info = Game_Map::GetMapInfo(map_id);
		return map_info.parent_map;
	};

	if (mode == 1) {
		var_id = game_variables->GetIndirect(var_id);
	}

	int value = -1;
	bool is_defined = game_variables->scoped_map.GetInherited(var_id, map_id, get_parent_map_id, value);
	if (!is_defined && game_variables->scoped_map.IsDefaultValueDefined(var_id, map_id)) {
		value = game_variables->scoped_map.GetDefaultValue(var_id);
	}
	return value;
}

namespace {
	constexpr int32_t PackIdsMapScopeConstant(int id, int map_id) {
		if (id <= 0 || id > DynamicScope::scopedvar_maps_max_count
			|| map_id < 0 || map_id > DynamicScope::scopedvar_max_map_id) {
			return 0;
		}
		return ((id - 1) << 2) + (map_id << 10);
	}

	constexpr int32_t PackIdsMapEvtScopeConstant(int id, int map_id, int evt_id) {
		if (id <= 0 || id > DynamicScope::scopedvar_mapevents_max_count
			|| map_id < 0 || map_id > DynamicScope::scopedvar_max_map_id
			|| evt_id < 0 || evt_id > DynamicScope::scopedvar_max_event_id) {
			return 0;
		}
		return ((id - 1) << 2) + (map_id + (evt_id * 10000) << 5);
	}

	constexpr int32_t PackIdsMapScopeVariable(int id, int mode, int map_var_id) {
		if (id <= 0 || id > DynamicScope::scopedvar_max_var_id_for_uint32_packing
			|| map_var_id < 0 || map_var_id > DynamicScope::scopedvar_max_var_id_for_uint32_packing) {
			return 0;
		}
		return mode + (id + (map_var_id * 10000)) << 2;
	}

	constexpr int32_t PackIdsMapEvtScopeVariable(int id, int mode, int map_var_id, int evt_var_id) {
		if (id <= 0 || id > DynamicScope::scopedvar_mapevents_max_count
			|| map_var_id < 0 || map_var_id > DynamicScope::scopedvar_max_var_id_for_uint32_packing
			|| evt_var_id < 0 || evt_var_id > DynamicScope::scopedvar_max_var_id_for_uint32_packing) {
			return 0;
		}
		return mode + ((id - 1) << 2) + (map_var_id + (evt_var_id * 10000) << 5);
	}
}

inline int32_t Game_Interpreter_Shared::PackMapScopedVarId(int id, int mode, int arg1) {
	assert(mode < SingleArgumentScopedVarPackingMode::eSingleArgScopePacking_Other);

	int32_t result = mode == SingleArgumentScopedVarPackingMode::eSingleArgScopePacking_Constant
		? PackIdsMapScopeConstant(id, arg1)
		: PackIdsMapScopeVariable(id, mode, arg1);
	if (result == 0) {
		Output::Debug("ScopedStorage (1 arg): Invalid id or out of range! (Map: {}, ScopedVarId: {}", arg1, id);
	}
	return result;
}

inline int32_t Game_Interpreter_Shared::PackMapEventScopedVarId(int id, int mode, int arg1, int arg2) {
	assert(mode < TwoArgumentScopedVarPackingMode::e2ArgsScopePacking_Other);

	int32_t result = mode == TwoArgumentScopedVarPackingMode::e2ArgsScopePacking_Constant
		? PackIdsMapEvtScopeConstant(id, arg1, arg2)
		: PackIdsMapEvtScopeVariable(id, mode, arg1, arg2);
	if (result == 0) {
		Output::Debug("ScopedStorage (2 args): Invalid id or out of range! (Map: {}, Evt: {}, ScopedVarId: {}", arg1, arg2, id);
	}
	return result;
}

inline bool Game_Interpreter_Shared::GetVariableIdByName(StringView variable_name, int& id) {
	// Not implemented
	return false;
}

inline bool Game_Interpreter_Shared::UnpackFrameScopedVarId(int packed_arg, int& id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
	int mode = packed_arg & 0x03;
	bool valid = false;

	switch (mode) {
		case eSingleArgScopePacking_Constant:
			id = ((packed_arg >> 2) & 0xFF) + 1;
			break;
		case eSingleArgScopePacking_Variable:
			id = game_variables->Get(((packed_arg & 0xFFFFFFE0) >> 2) % 10000);
			break;
		case eSingleArgScopePacking_VariableIndirect:
			id = game_variables->GetIndirect(((packed_arg & 0xFFFFFFE0) >> 2) % 10000);
			break;
		case eSingleArgScopePacking_Other:
		{
			int pos = 0;
			StringView var_name;
			int string_id;
			mode = (packed_arg >> 2) & 0x15;

			//26 bits remaining
			switch (mode) {
				case eSingleArgScopePacking_FrameVariable:
					id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
					break;
				case eSingleArgScopePacking_FrameVariableIndirect:
					id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
					id = game_variables->Get<eDataScope_Frame>(id, &interpreter.GetFrame());
					break;
				case eSingleArgScopePacking_Named:
					string_id = (packed_arg >> 6) & 0xFFFF;
					var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id, &pos, *game_variables);

					if (!GetVariableIdByName(var_name, id))
						return false;
					break;
				case eSingleArgScopePacking_NamedString:
					string_id = (packed_arg >> 6) & 0xFFFF;
					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id, *game_variables);

					if (!GetVariableIdByName(var_name, id))
						return false;
					break;
				case eSingleArgScopePacking_NamedStringIndirect:
					string_id = (packed_arg >> 6) & 0xFFFF;
					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id, *game_variables);

					if (!GetVariableIdByName(var_name, id))
						return false;
					break;
				default:
					return false;
			}
		}
		break;
	}

	valid = id > 0 && id <= DynamicScope::scopedvar_frame_max_count;

	if (!valid) {
		Output::Debug("FrameVar: Invalid id or out of range! {}", id);
		return false;
	}

	return true;
}

inline bool Game_Interpreter_Shared::UnpackMapScopedVarId(int packed_arg, int& id, int& map_id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
	int mode = packed_arg & 0x03;
	bool valid = false;

	switch (mode) {
		case eSingleArgScopePacking_Constant:
			id = ((packed_arg >> 2) & 0xFF) + 1;
			map_id = (packed_arg >> 10) & 0xFFFFF;
			break;
		case eSingleArgScopePacking_Variable:
			id = game_variables->Get(((packed_arg & 0xFFFFFFE0) >> 2) % 10000);
			map_id = game_variables->Get(((packed_arg & 0xFFFFFFE0) >> 2) / 10000);
			break;
		case eSingleArgScopePacking_VariableIndirect:
			id = game_variables->GetIndirect(((packed_arg & 0xFFFFFFE0) >> 2) % 10000);
			map_id = game_variables->GetIndirect(((packed_arg & 0xFFFFFFE0) >> 2) / 10000);
			break;
		case eSingleArgScopePacking_Other:
		{
			int pos = 0;
			StringView var_name;
			int string_id1, string_id2;
			mode = (packed_arg >> 2) & 0x15;

			//26 bits remaining
			switch (mode) {
				case eSingleArgScopePacking_FrameVariable:
					id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
					map_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 14) & 0xFFFF, &interpreter.GetFrame());
					break;
				case eSingleArgScopePacking_FrameVariableIndirect:
					id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
					id = game_variables->Get<eDataScope_Frame>(id, &interpreter.GetFrame());
					map_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 14) & 0xFFFF, &interpreter.GetFrame());
					map_id = game_variables->Get<eDataScope_Frame>(map_id, &interpreter.GetFrame());
					break;
				case eSingleArgScopePacking_Named:
					string_id1 = (packed_arg >> 6) & 0xFFFF;
					string_id2 = (packed_arg >> 14) & 0xFFFF;

					var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id1, &pos, *game_variables);
					if (!GetVariableIdByName(var_name, id))
						return false;
					var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id2, &pos, *game_variables);
					if (!GetVariableIdByName(var_name, map_id))
						return false;
					break;
				case eSingleArgScopePacking_NamedString:
					string_id1 = (packed_arg >> 6) & 0xFFFF;
					string_id2 = (packed_arg >> 14) & 0xFFFF;

					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id1, *game_variables);
					if (!GetVariableIdByName(var_name, id))
						return false;
					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id2, *game_variables);
					if (!GetVariableIdByName(var_name, map_id))
						return false;
					break;
				case eSingleArgScopePacking_NamedStringIndirect:
					string_id1 = (packed_arg >> 6) & 0xFFFF;
					string_id2 = (packed_arg >> 14) & 0xFFFF;

					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id1, *game_variables);
					if (!GetVariableIdByName(var_name, id))
						return false;
					var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id2, *game_variables);
					if (!GetVariableIdByName(var_name, map_id))
						return false;
					break;
				default:
					return false;
			}
		}
		break;
	}

	valid = id > 0 && id <= DynamicScope::scopedvar_maps_max_count
		&& map_id >= 0 && map_id <= DynamicScope::scopedvar_max_map_id;

	if (!valid) {
		Output::Debug("ScopedStorage (1 arg): Invalid id or out of range! (Map: {}, ScopedVarId: {})", map_id, id);
		return false;
	}

	if (map_id == 0)
		map_id = Game_Map::GetMapId();

	return true;
}

inline bool Game_Interpreter_Shared::UnpackMapEventScopedVarId(int packed_arg, int& id, int& map_id, int& evt_id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
	int mode = packed_arg & 0x03;
	id = ((packed_arg >> 2) & 0x7) + 1;
	bool valid = false;

	if (id > 0 || id <= DynamicScope::scopedvar_mapevents_max_count) {
		switch (mode) {
			case e2ArgsScopePacking_Constant:
				map_id = ((packed_arg & 0xFFFFFFE0) >> 5) % 10000;
				evt_id = ((packed_arg & 0xFFFFFFE0) >> 5) / 10000;
				valid = map_id >= 0 && map_id <= DynamicScope::scopedvar_max_map_id
					&& evt_id >= 0 || evt_id <= DynamicScope::scopedvar_max_event_id;
				break;
			case e2ArgsScopePacking_Variable:
				map_id = game_variables->Get(((packed_arg & 0xFFFFFFE0) >> 5) % 10000);
				evt_id = game_variables->Get(((packed_arg & 0xFFFFFFE0) >> 5) / 10000);
				valid = map_id >= 0 && map_id <= DynamicScope::scopedvar_max_map_id
					&& evt_id >= 0 || evt_id <= DynamicScope::scopedvar_max_event_id;
				break;
			case e2ArgsScopePacking_VariableIndirect:
				map_id = game_variables->GetIndirect(((packed_arg & 0xFFFFFFE0) >> 5) % 10000);
				evt_id = game_variables->GetIndirect(((packed_arg & 0xFFFFFFE0) >> 5) / 10000);
				valid = map_id >= 0 && map_id <= DynamicScope::scopedvar_max_map_id
					&& evt_id >= 0 || evt_id <= DynamicScope::scopedvar_max_event_id;
				break;
			case e2ArgsScopePacking_Other:
			{
				int pos = 0;
				StringView var_name;
				int string_id1, string_id2, string_id3;
				mode = (packed_arg >> 2) & 0x15;

				//26 bits remaining
				switch (mode) {
					case eSingleArgScopePacking_FrameVariable:
						id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
						map_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 14) & 0xFFFF, &interpreter.GetFrame());
						evt_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 22) & 0xFFFF, &interpreter.GetFrame());
						break;
					case eSingleArgScopePacking_FrameVariableIndirect:
						id = game_variables->Get<eDataScope_Frame>((packed_arg >> 6) & 0xFFFF, &interpreter.GetFrame());
						id = game_variables->Get<eDataScope_Frame>(id, &interpreter.GetFrame());
						map_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 14) & 0xFFFF, &interpreter.GetFrame());
						map_id = game_variables->Get<eDataScope_Frame>(map_id, &interpreter.GetFrame());
						evt_id = game_variables->Get<eDataScope_Frame>((packed_arg >> 22) & 0xFFFF, &interpreter.GetFrame());
						evt_id = game_variables->Get<eDataScope_Frame>(evt_id & 0xFFFF, &interpreter.GetFrame());
						break;
					case eSingleArgScopePacking_Named:
						string_id1 = (packed_arg >> 6) & 0xFFFF;
						string_id2 = (packed_arg >> 14) & 0xFFFF;
						string_id3 = (packed_arg >> 22) & 0xFFFF;

						var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id1, &pos, *game_variables);
						if (!GetVariableIdByName(var_name, id))
							return false;
						var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id2, &pos, *game_variables);
						if (!GetVariableIdByName(var_name, map_id))
							return false;
						var_name = game_strings->GetWithModeAndPos(com.string, Game_Strings::eStringEval_Text, string_id3, &pos, *game_variables);
						if (!GetVariableIdByName(var_name, evt_id))
							return false;
						break;
					case eSingleArgScopePacking_NamedString:
						string_id1 = (packed_arg >> 6) & 0xFFFF;
						string_id2 = (packed_arg >> 14) & 0xFFFF;
						string_id3 = (packed_arg >> 22) & 0xFFFF;

						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id1, *game_variables);
						if (!GetVariableIdByName(var_name, id))
							return false;
						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id2, *game_variables);
						if (!GetVariableIdByName(var_name, map_id))
							return false;
						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Direct, string_id3, *game_variables);
						if (!GetVariableIdByName(var_name, evt_id))
							return false;
						break;
					case eSingleArgScopePacking_NamedStringIndirect:
						string_id1 = (packed_arg >> 6) & 0xFFFF;
						string_id2 = (packed_arg >> 14) & 0xFFFF;
						string_id3 = (packed_arg >> 22) & 0xFFFF;

						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id1, *game_variables);
						if (!GetVariableIdByName(var_name, id))
							return false;
						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id2, *game_variables);
						if (!GetVariableIdByName(var_name, map_id))
							return false;
						var_name = game_strings->GetWithMode(com.string, Game_Strings::eStringEval_Indirect, string_id3, *game_variables);
						if (!GetVariableIdByName(var_name, evt_id))
							return false;
						break;
					default:
						return false;
				}
			}
			break;
		}
	}

	if (!valid) {
		Output::Debug("ScopedStorage (2 args): Invalid id or out of range! (Map: {}, Evt: {}, ScopedVarId: {})",
			map_id, evt_id, id);
		return false;
	}

	if (map_id == 0)
		map_id = Game_Map::GetMapId();

	if (evt_id == 0 || evt_id == Game_Character::CharThisEvent)
		evt_id = interpreter.GetThisEventId();

	return true;
}

//explicit declarations for target evaluation logic shared between ControlSwitches/ControlVariables/ControlStrings
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<true, false, false, false, false>(lcf::rpg::EventCommand const&, int&, int&) const;
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<true, true, true, false, false>(lcf::rpg::EventCommand const&, int&, int&) const;
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<false, true, false, true, false>(lcf::rpg::EventCommand const&, int&, int&) const;

//common variant for "Ex" commands
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<false, true, true, true, true, true>(lcf::rpg::EventCommand const&, int&, int&) const;

//explicit declarations for default value evaluation logic
template int Game_Interpreter_Shared::ValueOrVariable<true, true, false, false>(int, int, const Game_BaseInterpreterContext&);
template int Game_Interpreter_Shared::ValueOrVariableBitfield<true, true, false, false>(int, int, int, const Game_BaseInterpreterContext&);
template int Game_Interpreter_Shared::ValueOrVariableBitfield<true, true, false, false>(lcf::rpg::EventCommand const&, int, int, int, const Game_BaseInterpreterContext&);

//variant for "Ex" commands
template int Game_Interpreter_Shared::ValueOrVariableBitfield<false, true, true, true>(int, int, int, const Game_BaseInterpreterContext&);
