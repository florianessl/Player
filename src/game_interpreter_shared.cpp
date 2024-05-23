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
#include "game_switches.h"
#include "maniac_patch.h"
#include "main_data.h"

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
			break;
		}
		default:
		{
			if constexpr (support_scopes) {
				return false;
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
