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


#ifndef EP_GAME_INTERPRETER_SHARED
#define EP_GAME_INTERPRETER_SHARED

#include <lcf/rpg/eventcommand.h>
#include <lcf/rpg/saveeventexecframe.h>
#include <string_view.h>

class Game_Character;

class Game_BaseInterpreterContext {
public:
	virtual ~Game_BaseInterpreterContext() {}

	virtual int GetThisEventId() const = 0;
	virtual Game_Character* GetCharacter(int event_id) const = 0;
	virtual const lcf::rpg::SaveEventExecFrame& GetFrame() const = 0;

protected:
	template<bool validate_patches, bool support_range_indirect, bool support_expressions, bool support_bitmask, bool support_scopes, bool support_named = false>
	inline bool DecodeTargetEvaluationMode(lcf::rpg::EventCommand const& com, int& id_0, int& id_1) const {
		return Game_Interpreter_Shared::DecodeTargetEvaluationMode<validate_patches, support_range_indirect, support_expressions, support_bitmask, support_scopes, support_named>(com, id_0, id_1, *this);
	}

	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	inline int ValueOrVariable(int mode, int val) const {
		return Game_Interpreter_Shared::ValueOrVariable<validate_patches, support_indirect_and_switch, support_scopes, support_named>(mode, val, *this);
	}
	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	inline int ValueOrVariableBitfield(int mode, int shift, int val) const {
		return Game_Interpreter_Shared::ValueOrVariableBitfield<validate_patches, support_indirect_and_switch, support_scopes, support_named>(mode, shift, val, *this);
	}

	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	inline int ValueOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx) const {
		return Game_Interpreter_Shared::ValueOrVariableBitfield<validate_patches, support_indirect_and_switch, support_scopes, support_named>(com, mode_idx, shift, val_idx, *this);
	}
};

namespace Game_Interpreter_Shared {

	/*
	* Indicates how the target of an interpreter operation (lvalue) should be evaluated.
	*/
	enum TargetEvalMode : std::int8_t { // 4 bits
		eTargetEval_Single = 0,			// v[x]
		eTargetEval_Range = 1,			// v[x...y]
		eTargetEval_IndirectSingle = 2, // v[v[x]]
		eTargetEval_IndirectRange = 3,	// v[v[x]...v[y]] (ManiacPatch)
		eTargetEval_Expression = 4		// ManiacPatch expression
	};

	/*
	* Indicates how an operand of an interpreter operation (rvalue) should be evaluted.
	*/
	enum ValueEvalMode : std::int8_t { // 4 bits
		eValueEval_Constant = 0,				// Constant value is given
		eValueEval_Variable = 1,
		eValueEval_VariableIndirect = 2,
		eValueEval_Switch = 3,
		eValueEval_SwitchIndirect = 4
	};

	/*
	* Decodes how the target of an interpreter operation (or 'lvalue' in programming terms) should be evaluated
	* and returns the extracted variable ids.
	*
	* @tparam validate_patches Indicates if this template variant should validate for patch support (Maniac, EasyRPG-Commands)
	* @tparam support_range_indirect Indicates if indirect range operations should be supported (ManiacPatch)
	* @tparam support_expressions Indicates if Expressions (Mode=4) should be supported (ManiacPatch)
	* @tparam support_bitmask Indicates if the target mode should be derived from a bitmask (LSB bits) of the mode parameter
		(ManiacPatch string command encodes more info about other arguments in com.parameters[0])
	* @tparam support_scopes Indicates if ScopedVariables should be supported (EasyRPG-Commands - not implemented yet)
	* @tparam support_named Indicates if NamedVariables should be supported (EasyRPG-Commands - not implemented yet)
	* @param com \
	*	com.parameters[0]: Modifier for the operation target (lvalue) -> @see TargetEvalMode
	*	com.parameters[1]: The value from which id_0 (start) is decoded. This can be a constant, a variable id, or for special modes, packed information about scoped or named variables.
	*	com.parameters[2]: The value from which id_1 (end) is decoded, in case the operation is a range op. (see com.parameters[1])
	* @param id_0 Output parameter for the decoded, first variable id
	* @param id_1 Output parameter for the decoded, second variable id (for range operations)
	* @return True if the the parameters were successfully decoded
	*/
	template<bool validate_patches, bool support_range_indirect, bool support_expressions, bool support_bitmask, bool support_scopes, bool support_named = false>
	bool DecodeTargetEvaluationMode(lcf::rpg::EventCommand const& com, int& id_0, int& id_1, Game_BaseInterpreterContext const& interpreter);

	/*
	* Decodes how an operand of an interpreter operation (or 'rvalue' in programming terms) should be evaluated
	* and returns the its value
	*
	* @tparam validate_patches Indicates if this template variant should validate for patch support (Maniac, EasyRPG-Commands)
	* @tparam support_indirect_and_switch Indicates if indirect and switch evaluation should be supported (ManiacPatch)
	* @tparam support_scopes Indicates if ScopedVariables should be supported (EasyRPG-Commands - not implemented yet)
	* @tparam support_named Indicates if NamedVariables should be supported (EasyRPG-Commands - not implemented yet)
	* @param mode Modifier for the operand target (rvalue) -> @see ValueEvalMode
	* @param val The raw value extracted from the interpreter command which is to be evaluated
	*/
	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	int ValueOrVariable(int mode, int val, Game_BaseInterpreterContext const& interpreter);
	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	int ValueOrVariableBitfield(int mode, int shift, int val, Game_BaseInterpreterContext const& interpreter);
	// Range checked, conditional version (slower) of ValueOrVariableBitfield
	template<bool validate_patches = true, bool support_indirect_and_switch = true, bool support_scopes = false, bool support_named = false>
	int ValueOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx, Game_BaseInterpreterContext const& interpreter);

	StringView CommandStringOrVariable(lcf::rpg::EventCommand const& com, int mode_idx, int val_idx);
	StringView CommandStringOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx);

	bool CheckOperator(int val, int val2, int op);
}

inline bool Game_Interpreter_Shared::CheckOperator(int val, int val2, int op) {
	switch (op) {
		case 0:
			return val == val2;
		case 1:
			return val >= val2;
		case 2:
			return val <= val2;
		case 3:
			return val > val2;
		case 4:
			return val < val2;
		case 5:
			return val != val2;
		default:
			return false;
	}
}

//explicit declarations for target evaluation logic shared between ControlSwitches/ControlVariables/ControlStrings
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<true, false, false, false, false>(lcf::rpg::EventCommand const&, int&, int&) const;
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<true, true, true, false, false>(lcf::rpg::EventCommand const&, int&, int&) const;
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<false, true, false, true, false>(lcf::rpg::EventCommand const&, int&, int&) const;

//common variant for suggested "Ex" commands
template bool Game_BaseInterpreterContext::DecodeTargetEvaluationMode<false, true, true, true, true, true>(lcf::rpg::EventCommand const&, int&, int&) const;

//explicit declarations for default value evaluation logic
template int Game_Interpreter_Shared::ValueOrVariable<true, true, false, false>(int, int, const Game_BaseInterpreterContext&);
template int Game_Interpreter_Shared::ValueOrVariableBitfield<true, true, false, false>(int, int, int, const Game_BaseInterpreterContext&);
template int Game_Interpreter_Shared::ValueOrVariableBitfield<true, true, false, false>(lcf::rpg::EventCommand const&, int, int, int, const Game_BaseInterpreterContext&);

//variant for "Ex" commands
template int Game_Interpreter_Shared::ValueOrVariableBitfield<false, true, true, true>(int, int, int, const Game_BaseInterpreterContext&);

#endif
