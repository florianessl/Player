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
#include "game_scoped_storage.h"

class Game_Character;
class Game_BaseInterpreterContext;

namespace Game_Interpreter_Shared {

	/*
	* Indicates how the target of an interpreter operation (lvalue) should be evaluated.
	*/
	enum TargetEvalMode : std::int8_t { // 4 bits
		eTargetEval_Single = 0,			// v[x]
		eTargetEval_Range = 1,			// v[x...y]
		eTargetEval_IndirectSingle = 2, // v[v[x]]
		eTargetEval_IndirectRange = 3,	// v[v[x]...v[y]] (ManiacPatch)
		eTargetEval_Expression = 4,		// ManiacPatch expression


		// Evaluation mode is encoded in com.parameters[2] ('end_id'/'y' parameter usually used for specifying ranges)
		// @see TargetEvalExtMode
		eTargetEval_CodedInParam2 = 9,

		// Accessing Frame local variable.
		//  --> vframe[x]
		eTargetEval_FrameScope_IndirectSingle = 10,
		// Accessing a range of Frame local variables.
		//  --> v[vframe[x]...vframe[y]]
		eTargetEval_FrameScope_IndirectRange = 11,

		// ScopedVar access. Variable id is evaluated from a Map variable.
		//  --> v[vMap[x, map_id]]
		eTargetEval_MapScope_IndirectSingle = 12,
		// ScopedVar access. Variable range ids are evaluated from Map variables.
		//  --> v[vMap[x, map_id]...vMap[y, map_id]]
		eTargetEval_MapScope_IndirectRange = 13,

		// ScopedVar access. Variable id is evaluated from a MapEvent ('SelfVar') variable.
		//  --> v[vMapEvt[x, map_id, evt_id]]
		eTargetEval_MapEventScope_IndirectSingle = 14,
		// ScopedVar access. Variable range ids are evaluated from MapEvent ('SelfVar') variables.
		//  --> v[vMapEvt[x, map_id, evt_id]...vMapEvt[y, map_id, evt_id]]
		eTargetEval_MapEventScope_IndirectRange = 15
	};
	// Another use case might be a "void" parameter in case this mode specifier is used for a hypothetical
	//   new "CallFunction" feature to refer to a return/out parameters. (or just use mode=0 + id = 0 for void?)

	enum TargetEvalExtMode : std::int8_t {
		// NamedVar access. Variable name is part of command arguments.
		//  --> v[com.string]
		eTargetEvalExt_Named = 0,
		// NamedVar access. Variable name referenced by string x
		//  --> v[t[x]]
		eTargetEvalExt_NamedString = 1,
		// NamedVar access. Variable name referenced indirectly by string v[x]
		//  --> v[t[v[x]]]
		eTargetEvalExt_NamedStringIndirect = 2,

	};
	/*
	* Indicates how an operand of an interpreter operation (rvalue) should be evaluted.
	*/
	enum ValueEvalMode : std::int8_t { // 4 bits
		eValueEval_Constant = 0,				// Constant value is given
		eValueEval_Variable = 1,
		eValueEval_VariableIndirect = 2,
		eValueEval_Switch = 3,
		eValueEval_SwitchIndirect = 4,

		// 5 - 10 are Unused
		// Reserve space for Expression, NamedVariables (3x) ?

		eValueEval_FrameScopeVariable = 11,
		eValueEval_MapScopeVariable = 12,
		eValueEval_MapScopeVariableIndirect = 13,
		eValueEval_MapEventScopeVariable = 14,
		eValueEval_MapEventScopeVariableIndirect = 15,
	};

	/*
	* Indicates how a scoped variable that takes a single argument is encoded into a uint32
	* (Map level scopes)
	* Note:
	* - The (global) variable ids for modes 'Variable' or 'VariableIndirect' cannot
	*	exceed 9999!
	*/
	enum SingleArgumentScopedVarPackingMode : std::int8_t { // 2 bits
		eSingleArgScopePacking_Constant = 0,
		eSingleArgScopePacking_Variable = 1,
		eSingleArgScopePacking_VariableIndirect = 2,
		eSingleArgScopePacking_Other = 3
		//END
	};

	enum SingleArgumentScopedVarPackingModeExt : std::int8_t { // 4 bits
		eSingleArgScopePacking_FrameVariable = 0,
		eSingleArgScopePacking_FrameVariableIndirect = 1,
		eSingleArgScopePacking_Named = 2,
		eSingleArgScopePacking_NamedString = 3,
		eSingleArgScopePacking_NamedStringIndirect = 4
	};

	/*
	* Indicates how a scoped variable that takes two arguments is encoded into a uint32
	* (MapEvt scope aka. 'SelfSwitch' / 'SelfVar')
	* Note:
	* - The scoped variable range for this type of data is limited to '8' & only
	*	the two argument components can be referenced by a global variable
	*	(directly or indirectly).
	* - The 'id' part (1-8) is always constant for these types of ScopedVars
	* - The (global) variable ids for modes 'Variable' or 'VariableIndirect' cannot
	*	exceed 9999!
	*/
	enum TwoArgumentScopedVarPackingMode : std::int8_t { // 2 bits
		e2ArgsScopePacking_Constant = 0,
		e2ArgsScopePacking_Variable = 1,
		e2ArgsScopePacking_VariableIndirect = 2,
		e2ArgsScopePacking_Other = 3
		//END
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
	* @tparam support_scopes Indicates if ScopedVariables should be supported (EasyRPG-Commands)
	* @tparam support_named Indicates if NamedVariables should be supported (EasyRPG-Commands)
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
	* @tparam support_scopes Indicates if ScopedVariables should be supported (EasyRPG-Commands)
	* @tparam support_named Indicates if NamedVariables should be supported (EasyRPG-Commands)
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

	int scopedValueOrVariable(int mode, int val, int map_id, int event_id = 0);

	StringView CommandStringOrVariable(lcf::rpg::EventCommand const& com, int mode_idx, int val_idx);
	StringView CommandStringOrVariableBitfield(lcf::rpg::EventCommand const& com, int mode_idx, int shift, int val_idx);

	bool CheckOperator(int val, int val2, int op);

	game_bool EvaluateMapTreeSwitch(int mode, int switch_id, int map_id);
	int EvaluateMapTreeVariable(int mode, int var_id, int map_id);

	int32_t PackMapScopedVarId(int id, int mode, int arg1);
	int32_t PackMapEventScopedVarId(int id, int mode, int arg1, int arg2);
	//int32_t PackMapScopedVarWithNames(StringView varname);

	bool GetVariableIdByName(StringView variable_name, int& id);
	bool UnpackFrameScopedVarId(int packed_arg, int& id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter);
	bool UnpackMapScopedVarId(int packed_arg, int& id, int& map_id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter);
	bool UnpackMapEventScopedVarId(int packed_arg, int& id, int& map_id, int& evt_id, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter);
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

#endif
