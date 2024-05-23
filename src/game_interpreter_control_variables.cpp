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

#ifndef EP_GAME_INTERPRETER_CONTROL_VARIABLES_IMPL
#define EP_GAME_INTERPRETER_CONTROL_VARIABLES_IMPL

#include "compiler.h"
#include "game_interpreter_shared.h"
#include "game_interpreter_control_variables.h"
#include "game_actors.h"
#include "game_enemyparty.h"
#include "game_ineluki.h"
#include "game_party.h"
#include "game_player.h"
#include "game_system.h"
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


namespace ControlSwitches {
	void PerformSwitchOp(int operation, int switch_id);

	void PerformSwitchRangeOp(int operation, int start, int end);
}

namespace ControlVariables {

	enum ControlVarOperand : std::uint8_t {
		// Constant
		eVarOperand_Constant = 0,
		// Var A ops B
		eVarOperand_Variable,
		// Number of var A ops B
		eVarOperand_VariableIndirect,
		// Random between range
		eVarOperand_RandomBetweenRange,
		// Items
		eVarOperand_Items,
		// Hero
		eVarOperand_Actors,
		// Characters
		eVarOperand_Events,
		// More
		eVarOperand_Other,
		// Battle related
		eVarOperand_Battle_Enemies,
		eVarOperand_Maniacs_Party = 9,
		eVarOperand_Maniacs_Switch,
		eVarOperand_Maniacs_Pow,
		eVarOperand_Maniacs_Sqrt,
		eVarOperand_Maniacs_Sin,
		eVarOperand_Maniacs_Cos,
		eVarOperand_Maniacs_Atan2,
		eVarOperand_Maniacs_Min,
		eVarOperand_Maniacs_Max,
		eVarOperand_Maniacs_Abs,
		eVarOperand_Maniacs_Binary,
		eVarOperand_Maniacs_Ternary,
		eVarOperand_Maniacs_Expression,

		// Reserved / Maniacs 2024-xx-xx (not yet implemented - need to determine encoding/functionality + actual op ids)
		eVarOperand_Maniacs_Clamp,
		eVarOperand_Maniacs_MulDiv,
		eVarOperand_Maniacs_DivMul,
		eVarOperand_Maniacs_Between,
		eVarOperand_Maniacs_Lerp,
		eVarOperand_Maniacs_SumRange,
		eVarOperand_Maniacs_AMin,
		eVarOperand_Maniacs_AMax,
		//

		eVarOperand_EasyRpg_FrameSwitch = 200,
		eVarOperand_EasyRpg_ScopedSwitch_Map,
		eVarOperand_EasyRpg_ScopedSwitch_MapEvent,
		eVarOperand_EasyRpg_FrameVariable,
		eVarOperand_EasyRpg_ScopedVariable_Map,
		eVarOperand_EasyRpg_ScopedVariable_MapEvent,
		// Count Switches [id] matching condition (ON/OFF) or defined (arg>=2) (Scope: Map)
		eVarOperand_EasyRpg_CountScopedSwitchesMatchingCondition_Map,
		// Count Switches [id] matching condition (ON/OFF) or defined (arg>=2) for map [map_id] (Scope: MapEvent)
		eVarOperand_EasyRpg_CountScopedSwitchesMatchingCondition_MapEvent,
		// Count Variables [id] matching condition or defined (op>=6) (Scope: Map)
		eVarOperand_EasyRpg_CountScopedVarsMatchingCondition_Map,
		// Count Variables [id] matching condition or defined (op>=6) for map [map_id]  (Scope: MapEvent)
		eVarOperand_EasyRpg_CountScopedVarsMatchingCondition_MapEvent,

		// Reserved / Not yet implemented
		eVarOperand_EasyRpg_DateTime,		// jetrotals new dt operations
		eVarOperand_EasyRpg_MapInfo,		// jetrotals planned ops for Map/Screen ?
		eVarOperand_EasyRpg_MessageState,	// get info about active message windows
		eVarOperand_EasyRpg_RngFixedSeed,	// reproducible rng (+ options for rng dependent on map/event ?)
		//

		eVarOperand_Vanilla_FIRST = eVarOperand_Constant,
		eVarOperand_Vanilla_LAST = eVarOperand_Battle_Enemies,
		eVarOperand_Maniacs_FIRST = eVarOperand_Maniacs_Party,
		eVarOperand_Maniacs_LAST = eVarOperand_Maniacs_Expression,
		eVarOperand_Maniacs24xxxx_FIRST = eVarOperand_Maniacs_Clamp,
		eVarOperand_Maniacs24xxxx_LAST = eVarOperand_Maniacs_AMax,
		eVarOperand_EasyRpg_FIRST = eVarOperand_EasyRpg_FrameSwitch,
		eVarOperand_EasyRpg_LAST = eVarOperand_EasyRpg_RngFixedSeed,

		eVarOperand_MAX = eVarOperand_EasyRpg_LAST
	};

	using varOperand_Func = int (*)(lcf::rpg::EventCommand const&, Game_BaseInterpreterContext const&);

	static bool dispatch_table_default_case_triggered = false;

	//dispatch table
	class dispatch_table_varoperand {
	public:
		dispatch_table_varoperand(const int param_operand, const int patch_flags, const std::map<ControlVarOperand, varOperand_Func> defined_ops, varOperand_Func default_case) : param_operand(param_operand), patch_flags(patch_flags), ops(InitOps(defined_ops, default_case)) { }

		bool Execute(int& value_out, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) const;

		inline int GetPatchFlags() const { return patch_flags; }
	private:
		static constexpr int table_size = { (int)std::numeric_limits<unsigned char>::max()};

		static inline std::array<varOperand_Func, table_size> InitOps(const std::map<ControlVarOperand, varOperand_Func> defined_ops, varOperand_Func default_case) {
			std::array<varOperand_Func, table_size> ret;

			for (int i = 0; i < table_size; i++) {
				auto it = defined_ops.find(static_cast<ControlVarOperand>(std::byte(i)));
				if (it == defined_ops.end()) {
					ret[i] = default_case;
				} else {
					ret[i] = it->second;
				}
			}

			return ret;
		}

		const int param_operand, patch_flags;
		const std::array<varOperand_Func, table_size> ops;
	};

	enum ControlVarCommandsType {
		eControlVarOp_Default = 0,
		eControlVarOp_Ex,
		eControlVarOp_Scoped,
		eControlVarOp_LAST
	};

	std::array<dispatch_table_varoperand*, eControlVarOp_LAST> tables = {};

	constexpr StringView get_op_name(ControlVarCommandsType op_type) {
		switch (op_type) {
			case ControlVarCommandsType::eControlVarOp_Default:
				return "ControlVariables";
			case ControlVarCommandsType::eControlVarOp_Ex:
				return "ControlVariablesEx";
			case ControlVarCommandsType::eControlVarOp_Scoped:
				return "ControlScopedVariables";
			default:
				return "";
		}
	};

	constexpr int get_param_operand(ControlVarCommandsType op_type) {
		switch (op_type) {
			case ControlVarCommandsType::eControlVarOp_Default:
				return 4;
			case ControlVarCommandsType::eControlVarOp_Ex:
				return 4;
			case ControlVarCommandsType::eControlVarOp_Scoped:
				return 4;
			default:
				return 4;
		}
	};

	constexpr int get_param_offset(ControlVarCommandsType op_type) {
		switch (op_type) {
			case ControlVarCommandsType::eControlVarOp_Default:
				return 5;
			case ControlVarCommandsType::eControlVarOp_Ex:
				return 5;
			case ControlVarCommandsType::eControlVarOp_Scoped:
				return 8;
			default:
				return 5;
		}
	};

	template <ControlVarCommandsType op_type>
	const dispatch_table_varoperand& BuildDispatchTable(const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx);

	void RebuildDispatchTables(const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx);

	void PerformVarOp(int operation, int var_id, int value);
	void PerformVarRangeOp(int operation, int start, int end, int value);
	void PerformVarRangeOpVariable(int operation, int start, int end, int var_id);
	void PerformVarRangeOpVariableIndirect(int operation, int start, int end, int var_id);
	void PerformVarRangeOpRandom(int operation, int start, int end, int rmin, int max);
}

namespace ConditionalBranching {
	enum ConditionalBranch : std::uint8_t {
		eCondition_Switch = 0,
		eCondition_Variable,
		eCondition_Timer,
		eCondition_Gold,
		eCondition_Item,
		eCondition_Hero,
		eCondition_CharOrientation,
		eCondition_VehicleInUse,
		eCondition_TriggeredByDecisionKey,
		eCondition_BgmLoopedOnce,
		eCondition_2k3_Timer2,
		eCondition_2k3_Other,
		eCondition_Maniacs_Other = 12,
		eCondition_Maniacs_SwitchIndirect,
		eCondition_Maniacs_VariableIndirect,
		eCondition_Maniacs_StringComparison,
		eCondition_Maniacs_Expression,

		eCondition_EasyRpg_FrameSwitch = 200,
		eCondition_EasyRpg_ScopedSwitch_Map,
		eCondition_EasyRpg_ScopedSwitch_MapEvent,
		eCondition_EasyRpg_FrameVariable,
		eCondition_EasyRpg_ScopedVariable_Map,
		eCondition_EasyRpg_ScopedVariable_MapEvent,

		eCondition_Vanilla_FIRST = eCondition_Switch,
		eCondition_Vanilla_LAST = eCondition_2k3_Other,
		eCondition_Maniacs_FIRST = eCondition_Maniacs_Other,
		eCondition_Maniacs_LAST = eCondition_Maniacs_Expression,
		eCondition_EasyRpg_FIRST = eCondition_EasyRpg_FrameSwitch,
		eCondition_EasyRpg_LAST = eCondition_EasyRpg_ScopedVariable_MapEvent,

		eCondition_MAX = eCondition_EasyRpg_LAST
	};

	using condition_Func = bool (*)(lcf::rpg::EventCommand const&, Game_BaseInterpreterContext const&);

	static bool dispatch_table_default_case_triggered = false;

	class dispatch_table_condition {
	public:
		dispatch_table_condition(const int patch_flags, const std::map<ConditionalBranch, condition_Func> defined_ops, condition_Func default_case) : patch_flags(patch_flags), ops(InitOps(defined_ops, default_case)) {}

		bool Execute(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) const;

		inline int GetPatchFlags() const { return patch_flags; }
	private:
		static constexpr int table_size = { (int)std::numeric_limits<unsigned char>::max() };

		static inline std::array<condition_Func, table_size> InitOps(const std::map<ConditionalBranch, condition_Func> defined_ops, condition_Func default_case) {
			std::array<condition_Func, table_size> ret;

			for (int i = 0; i < table_size; i++) {
				auto it = defined_ops.find(static_cast<ConditionalBranch>(std::byte(i)));
				if (it == defined_ops.end()) {
					ret[i] = default_case;
				} else {
					ret[i] = it->second;
				}
			}

			return ret;
		}

		const int patch_flags;
		const std::array<condition_Func, table_size> ops;
	};

	enum CondBranchCommandsType {
		eCondBranch_Default = 0,
		eCondBranch_Ex,
		eCondBranch_LAST
	};

	std::array<dispatch_table_condition*, eCondBranch_LAST> tables = {};

	constexpr StringView get_op_name(CondBranchCommandsType op_type) {
		switch (op_type) {
			case CondBranchCommandsType::eCondBranch_Default:
				return "ConditionalBranch";
			case CondBranchCommandsType::eCondBranch_Ex:
				return "ConditionalBranchEx";
			default:
				return "";
		}
	};

	template <CondBranchCommandsType op_type>
	dispatch_table_condition& BuildDispatchTable(const bool include2k3Commands, const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx);

	void RebuildDispatchTables(const bool include2k3Commands, const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx);
}

using Main_Data::game_switches, Main_Data::game_variables, Main_Data::game_strings;

using Main_Data::game_party, Main_Data::game_actors, Main_Data::game_ineluki, Main_Data::game_enemyparty, Main_Data::game_system;

inline void ControlSwitches::PerformSwitchOp(int operation, int switch_id) {
	if (operation < 2) {
		game_switches->Set(switch_id, operation == 0);
	} else {
		game_switches->Flip(switch_id);
	}
}

inline void ControlSwitches::PerformSwitchRangeOp(int operation, int start, int end) {
	if (operation < 2) {
		game_switches->SetRange(start, end, operation == 0);
	} else {
		game_switches->FlipRange(start, end);
	}
}

using namespace Game_Interpreter_Shared;

/* Template Definitions */
namespace ControlVariables {

	template <ControlVarCommandsType op_type>
	int varOperand_DefaultCase(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const&) {
		dispatch_table_default_case_triggered = true;
		Output::Warning("{}: Unsupported operand {}", get_op_name(op_type), com.parameters[get_param_operand(op_type)]);
		return 0;
	};

	template <int param_offset>
	int Constant(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		return com.parameters[param_offset];
	}

	template <int param_offset>
	int Variable(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		return game_variables->Get(com.parameters[param_offset]);
	}

	template <int param_offset>
	int VariableIndirect(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		return game_variables->GetIndirect(com.parameters[param_offset]);
	}

	template <int param_offset, bool Maniac>
	int Random(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int32_t arg1 = com.parameters[param_offset];
		int32_t arg2 = com.parameters[param_offset + 1];
		if constexpr (Maniac) {
			if (com.parameters.size() >= (param_offset + 3)) {
				arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, arg1, interpreter);
				arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, arg2, interpreter);
			}
		}

		return Random(arg1, arg2);
	}

	template <int param_offset, bool Maniac>
	int Item(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int item = com.parameters[param_offset];
		if constexpr (Maniac) {
			if (com.parameters.size() >= (param_offset + 3)) {
				item = ValueOrVariable(com.parameters[param_offset + 2], item, interpreter);
			}
		}

		return Item(com.parameters[param_offset + 1], item);
	}

	template <int param_offset, bool Maniac>
	int Actor(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int actor_id = com.parameters[param_offset];
		if constexpr (Maniac) {
			if (com.parameters.size() >= (param_offset + 3)) {
				actor_id = ValueOrVariable(com.parameters[param_offset + 2], actor_id, interpreter);
			}
		}
		return Actor<Maniac>(com.parameters[param_offset + 1], actor_id);
	}

	template <int param_offset, bool Maniac>
	int Event(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int event_id = com.parameters[param_offset];
		if constexpr (Maniac) {
			if (com.parameters.size() >= (param_offset + 3)) {
				event_id = ValueOrVariable(com.parameters[param_offset + 2], event_id, interpreter);
			}
		}
		return Event<Maniac>(com.parameters[param_offset + 1], event_id, interpreter);
	}

	template <int param_offset, bool Maniac>
	int Other(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		return Other<Maniac>(com.parameters[param_offset]);
	}

	template <int param_offset, bool Maniac>
	int Enemy(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int enemy_id = com.parameters[param_offset];
		if constexpr (Maniac) {
			if (com.parameters.size() >= (param_offset + 3)) {
				enemy_id = ValueOrVariable(com.parameters[param_offset + 2], enemy_id, interpreter);
			}
		}

		return Enemy<Maniac>(com.parameters[param_offset + 1], enemy_id);
	}

	template <int param_offset>
	int Party(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int party_idx = com.parameters[param_offset];
		if (com.parameters.size() >= (param_offset + 3)) {
			party_idx = ValueOrVariable(com.parameters[param_offset + 2], party_idx, interpreter);
		}
		return Party(com.parameters[param_offset + 1], party_idx);
	}

	template <int param_offset>
	int Switch(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int value = com.parameters[param_offset];
		if (com.parameters[param_offset + 1] == 1) {
			value = game_switches->GetInt(value);
		} else {
			value = game_switches->GetInt(game_variables->Get(value));
		}
		return value;
	}

	template <int param_offset>
	int Pow(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, com.parameters[param_offset + 1], interpreter);
		return Pow(arg1, arg2);
	}

	template <int param_offset>
	int Sqrt(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int mul = com.parameters[param_offset + 1];
		return Sqrt(arg, mul);
	}

	template <int param_offset>
	int Sin(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, com.parameters[param_offset + 3], interpreter);
		float mul = static_cast<float>(com.parameters[param_offset + 1]);
		return Sin(arg1, arg2, mul);
	}

	template <int param_offset>
	int Cos(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, com.parameters[param_offset + 3], interpreter);
		int mul = com.parameters[param_offset + 1];
		return Cos(arg1, arg2, mul);
	}

	template <int param_offset>
	int Atan2(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 3], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 3], 1, com.parameters[param_offset + 1], interpreter);
		int mul = com.parameters[param_offset + 2];
		return Atan2(arg1, arg2, mul);
	}

	template <int param_offset>
	int Min(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, com.parameters[param_offset + 1], interpreter);
		return Min(arg1, arg2);
	}

	template <int param_offset>
	int Max(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 0, com.parameters[param_offset], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 2], 1, com.parameters[param_offset + 1], interpreter);
		return Max(arg1, arg2);
	}

	template <int param_offset>
	int Abs(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg = ValueOrVariableBitfield(com.parameters[param_offset + 1], 0, com.parameters[param_offset], interpreter);
		return Abs(arg);
	}

	template <int param_offset>
	int Binary(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int arg1 = ValueOrVariableBitfield(com.parameters[param_offset + 3], 0, com.parameters[param_offset + 1], interpreter);
		int arg2 = ValueOrVariableBitfield(com.parameters[param_offset + 3], 1, com.parameters[param_offset + 2], interpreter);
		return Binary(com.parameters[param_offset], arg1, arg2);
	}

	template <int param_offset>
	int Ternary(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		int mode = com.parameters[param_offset + 5];
		int arg1 = ValueOrVariableBitfield(mode, 0, com.parameters[param_offset + 1], interpreter);
		int arg2 = ValueOrVariableBitfield(mode, 1, com.parameters[param_offset + 2], interpreter);
		int op = com.parameters[param_offset];
		if (CheckOperator(arg1, arg2, op)) {
			return ValueOrVariableBitfield(mode, 2, com.parameters[param_offset + 3], interpreter);
		}
		return ValueOrVariableBitfield(mode, 3, com.parameters[param_offset + 4], interpreter);
	}

	template <int param_offset>
	static inline int Expression(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter)
	{
		return ManiacPatch::ParseExpression(MakeSpan(com.parameters).subspan(param_offset + 1, com.parameters[param_offset]), interpreter);
	}
}

EP_ALWAYS_INLINE bool ControlVariables::dispatch_table_varoperand::Execute(int& value_out, lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) const {

	const std::byte operand = static_cast<std::byte>(com.parameters[param_operand]);
	value_out = ops[std::to_integer<int>(operand)](com, interpreter);

	if (EP_UNLIKELY(dispatch_table_default_case_triggered)) {
		dispatch_table_default_case_triggered = false;
		return false;
	}
	return true;
}

template <ControlVariables::ControlVarCommandsType op_type>
const ControlVariables::dispatch_table_varoperand& ControlVariables::BuildDispatchTable(const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx) {

	static_assert(op_type >= eControlVarOp_Default && op_type < eControlVarOp_LAST);
	assert(tables[static_cast<int>(op_type)] == nullptr);

	std::bitset<16> patch_flags;
	patch_flags.set(1, includeManiacs_200128);
	patch_flags.set(2, includeManiacs24xxxx);
	patch_flags.set(3, includeEasyRpgEx);

	std::map<ControlVarOperand, varOperand_Func> ops;

	// Vanilla operands
	ops[eVarOperand_Constant] = &Constant<get_param_offset(op_type)>;
	ops[eVarOperand_Variable] = &Variable<get_param_offset(op_type)>;
	ops[eVarOperand_VariableIndirect] = &VariableIndirect<get_param_offset(op_type)>;
	if (includeManiacs_200128) {
		ops[eVarOperand_RandomBetweenRange] = &Random<get_param_offset(op_type), true>;
		ops[eVarOperand_Items] = &Item<get_param_offset(op_type), true>;
		ops[eVarOperand_Actors] = &Actor<get_param_offset(op_type), true>;
		ops[eVarOperand_Events] = &Event<get_param_offset(op_type), true>;
		ops[eVarOperand_Other] = &Other<get_param_offset(op_type), true>;
		ops[eVarOperand_Battle_Enemies] = &Enemy<get_param_offset(op_type), true>;
	} else {
		ops[eVarOperand_RandomBetweenRange] = &Random<get_param_offset(op_type), false>;
		ops[eVarOperand_Items] = &Item<get_param_offset(op_type), false>;
		ops[eVarOperand_Actors] = &Actor<get_param_offset(op_type), false>;
		ops[eVarOperand_Events] = &Event<get_param_offset(op_type), false>;
		ops[eVarOperand_Other] = &Other<get_param_offset(op_type), false>;
		ops[eVarOperand_Battle_Enemies] = &Enemy<get_param_offset(op_type), false>;
	}
	// end Vanilla operands

	if (includeManiacs_200128) {
		ops[eVarOperand_Maniacs_Party] = &Party<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Switch] = &Switch<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Pow] = &Pow<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Sqrt] = &Sqrt<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Sin] = &Sin<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Cos] = &Cos<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Atan2] = &Atan2<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Min] = &Min<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Max] = &Max<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Abs] = &Abs<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Binary] = &Binary<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Ternary] = &Ternary<get_param_offset(op_type)>;
		ops[eVarOperand_Maniacs_Expression] = &Expression<get_param_offset(op_type)>;
	}

	dispatch_table_varoperand* dispatch_table = new dispatch_table_varoperand(get_param_operand(op_type), (int)patch_flags.to_ulong(), ops, &varOperand_DefaultCase<op_type>);
	tables[static_cast<int>(op_type)] = dispatch_table;

	return *dispatch_table;
}

void ControlVariables::RebuildDispatchTables(const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx) {

	std::bitset<16> bitset;
	bitset.set(1, includeManiacs_200128);
	bitset.set(2, includeManiacs24xxxx);
	bitset.set(3, includeEasyRpgEx);

	const int patch_flags_new = (int)bitset.to_ulong();

	//note: with C++20 we could just write a templated lambda here

	if (tables[eControlVarOp_Default] != nullptr && tables[eControlVarOp_Default]->GetPatchFlags() != patch_flags_new) {
		delete tables[eControlVarOp_Default];
		tables[eControlVarOp_Default] = nullptr;

		BuildDispatchTable<eControlVarOp_Default>(includeManiacs_200128, includeManiacs24xxxx, includeEasyRpgEx);
	}
	if (tables[eControlVarOp_Scoped] != nullptr && tables[eControlVarOp_Scoped]->GetPatchFlags() != patch_flags_new) {
		delete tables[eControlVarOp_Scoped];
		tables[eControlVarOp_Scoped] = nullptr;

		BuildDispatchTable<eControlVarOp_Scoped>(includeManiacs_200128, includeManiacs24xxxx, includeEasyRpgEx);
	}
}

inline void ControlVariables::PerformVarOp(int operation, int var_id, int value) {
	switch (operation) {
		case 0:
			game_variables->Set(var_id, value);
			break;
		case 1:
			game_variables->Add(var_id, value);
			break;
		case 2:
			game_variables->Sub(var_id, value);
			break;
		case 3:
			game_variables->Mult(var_id, value);
			break;
		case 4:
			game_variables->Div(var_id, value);
			break;
		case 5:
			game_variables->Mod(var_id, value);
			break;
		case 6:
			game_variables->BitOr(var_id, value);
			break;
		case 7:
			game_variables->BitAnd(var_id, value);
			break;
		case 8:
			game_variables->BitXor(var_id, value);
			break;
		case 9:
			game_variables->BitShiftLeft(var_id, value);
			break;
		case 10:
			game_variables->BitShiftRight(var_id, value);
			break;
	}
}

inline void ControlVariables::PerformVarRangeOp(int operation, int start, int end, int value) {
	switch (operation) {
		case 0:
			game_variables->SetRange(start, end, value);
			break;
		case 1:
			game_variables->AddRange(start, end, value);
			break;
		case 2:
			game_variables->SubRange(start, end, value);
			break;
		case 3:
			game_variables->MultRange(start, end, value);
			break;
		case 4:
			game_variables->DivRange(start, end, value);
			break;
		case 5:
			game_variables->ModRange(start, end, value);
			break;
		case 6:
			game_variables->BitOrRange(start, end, value);
			break;
		case 7:
			game_variables->BitAndRange(start, end, value);
			break;
		case 8:
			game_variables->BitXorRange(start, end, value);
			break;
		case 9:
			game_variables->BitShiftLeftRange(start, end, value);
			break;
		case 10:
			game_variables->BitShiftRightRange(start, end, value);
			break;
	}
}

inline void ControlVariables::PerformVarRangeOpVariable(int operation, int start, int end, int var_id) {
	switch (operation) {
		case 0:
			game_variables->SetRangeVariable(start, end, var_id);
			break;
		case 1:
			game_variables->AddRangeVariable(start, end, var_id);
			break;
		case 2:
			game_variables->SubRangeVariable(start, end, var_id);
			break;
		case 3:
			game_variables->MultRangeVariable(start, end, var_id);
			break;
		case 4:
			game_variables->DivRangeVariable(start, end, var_id);
			break;
		case 5:
			game_variables->ModRangeVariable(start, end, var_id);
			break;
		case 6:
			game_variables->BitOrRangeVariable(start, end, var_id);
			break;
		case 7:
			game_variables->BitAndRangeVariable(start, end, var_id);
			break;
		case 8:
			game_variables->BitXorRangeVariable(start, end, var_id);
			break;
		case 9:
			game_variables->BitShiftLeftRangeVariable(start, end, var_id);
			break;
		case 10:
			game_variables->BitShiftRightRangeVariable(start, end, var_id);
			break;
	}
}

inline void ControlVariables::PerformVarRangeOpVariableIndirect(int operation, int start, int end, int var_id) {
	switch (operation) {
		case 0:
			game_variables->SetRangeVariableIndirect(start, end, var_id);
			break;
		case 1:
			game_variables->AddRangeVariableIndirect(start, end, var_id);
			break;
		case 2:
			game_variables->SubRangeVariableIndirect(start, end, var_id);
			break;
		case 3:
			game_variables->MultRangeVariableIndirect(start, end, var_id);
			break;
		case 4:
			game_variables->DivRangeVariableIndirect(start, end, var_id);
			break;
		case 5:
			game_variables->ModRangeVariableIndirect(start, end, var_id);
			break;
		case 6:
			game_variables->BitOrRangeVariableIndirect(start, end, var_id);
			break;
		case 7:
			game_variables->BitAndRangeVariableIndirect(start, end, var_id);
			break;
		case 8:
			game_variables->BitXorRangeVariableIndirect(start, end, var_id);
			break;
		case 9:
			game_variables->BitShiftLeftRangeVariableIndirect(start, end, var_id);
			break;
		case 10:
			game_variables->BitShiftRightRangeVariableIndirect(start, end, var_id);
			break;
	}
}

inline void ControlVariables::PerformVarRangeOpRandom(int operation, int start, int end, int rmin, int rmax) {
	switch (operation) {
		case 0:
			game_variables->SetRangeRandom(start, end, rmin, rmax);
			break;
		case 1:
			game_variables->AddRangeRandom(start, end, rmin, rmax);
			break;
		case 2:
			game_variables->SubRangeRandom(start, end, rmin, rmax);
			break;
		case 3:
			game_variables->MultRangeRandom(start, end, rmin, rmax);
			break;
		case 4:
			game_variables->DivRangeRandom(start, end, rmin, rmax);
			break;
		case 5:
			game_variables->ModRangeRandom(start, end, rmin, rmax);
			break;
		case 6:
			game_variables->BitOrRangeRandom(start, end, rmin, rmax);
			break;
		case 7:
			game_variables->BitAndRangeRandom(start, end, rmin, rmax);
			break;
		case 8:
			game_variables->BitXorRangeRandom(start, end, rmin, rmax);
			break;
		case 9:
			game_variables->BitShiftLeftRangeRandom(start, end, rmin, rmax);
			break;
		case 10:
			game_variables->BitShiftRightRangeRandom(start, end, rmin, rmax);
			break;
	}
}

int ControlVariables::Random(int value, int value2) {
	int rmax = std::max(value, value2);
	int rmin = std::min(value, value2);

	return Rand::GetRandomNumber(rmin, rmax);
}

int ControlVariables::Item(int op, int item) {
	switch (op) {
		case 0:
			// Number of items posessed
			return game_party->GetItemCount(item);
			break;
		case 1:
			// How often the item is equipped
			return game_party->GetEquippedItemCount(item);
			break;
	}

	Output::Warning("ControlVariables::Item: Unknown op {}", op);
	return 0;
}

template<bool ManiacPatch>
int ControlVariables::Actor(int op, int actor_id) {
	auto actor = game_actors->GetActor(actor_id);

	if (!actor) {
		Output::Warning("ControlVariables::Actor: Bad actor_id {}", actor_id);
		return 0;
	}

	switch (op) {
		case 0:
			// Level
			return actor->GetLevel();
			break;
		case 1:
			// Experience
			return actor->GetExp();
			break;
		case 2:
			// Current HP
			return actor->GetHp();
			break;
		case 3:
			// Current MP
			return actor->GetSp();
			break;
		case 4:
			// Max HP
			return actor->GetMaxHp();
			break;
		case 5:
			// Max MP
			return actor->GetMaxSp();
			break;
		case 6:
			// Attack++ pas
			return actor->GetAtk();
			break;
		case 7:
			// Defense
			return actor->GetDef();
			break;
		case 8:
			// Intelligence
			return actor->GetSpi();
			break;
		case 9:
			// Agility
			return actor->GetAgi();
			break;
		case 10:
			// Weapon ID
			return actor->GetWeaponId();
			break;
		case 11:
			// Shield ID
			return actor->GetShieldId();
			break;
		case 12:
			// Armor ID
			return actor->GetArmorId();
			break;
		case 13:
			// Helmet ID
			return actor->GetHelmetId();
			break;
		case 14:
			// Accessory ID
			return actor->GetAccessoryId();
			break;
		case 15:
			// ID
			if constexpr (ManiacPatch) {
				return actor->GetId();
			}
			break;
		case 16:
			// ATB
			if constexpr (ManiacPatch) {
				return actor->GetAtbGauge();
			}
			break;
	}

	Output::Warning("ControlVariables::Actor: Unknown op {}", op);
	return 0;
}

int ControlVariables::Party(int op, int party_idx) {
	auto actor = game_party->GetActor(party_idx);

	if (!actor) {
		Output::Warning("ControlVariables::Party: Bad party_idx {}", party_idx);
		return 0;
	}

	return ControlVariables::Actor<true>(op, actor->GetId());
}

template<bool ManiacPatch>
int ControlVariables::Event(int op, int event_id, const Game_BaseInterpreterContext& interpreter) {
	auto character = interpreter.GetCharacter(event_id);
	if (character) {
		switch (op) {
			case 0:
				// Map ID
				if (!Player::IsRPG2k()
					|| event_id == Game_Character::CharPlayer
					|| event_id == Game_Character::CharBoat
					|| event_id == Game_Character::CharShip
					|| event_id == Game_Character::CharAirship) {
					return character->GetMapId();
				} else {
					// This is an RPG_RT bug for 2k only. Requesting the map id of an event always returns 0.
					return 0;
				}
				break;
			case 1:
				// X Coordinate
				return character->GetX();
				break;
			case 2:
				// Y Coordinate
				return character->GetY();
				break;
			case 3:
				// Orientation
				int dir;
				dir = character->GetFacing();
				return dir == 0 ? 8 :
					dir == 1 ? 6 :
					dir == 2 ? 2 : 4;
				break;
			case 4: {
				// Screen X
				if (Player::game_config.fake_resolution.Get()) {
					int pan_delta = (Game_Player::GetDefaultPanX() - lcf::rpg::SavePartyLocation::kPanXDefault) / TILE_SIZE;
					return character->GetScreenX() - pan_delta;
				} else {
					return character->GetScreenX();
				}
			}
			case 5: {
				// Screen Y
				if (Player::game_config.fake_resolution.Get()) {
					int pan_delta = (Game_Player::GetDefaultPanY() - lcf::rpg::SavePartyLocation::kPanYDefault) / TILE_SIZE;
					return character->GetScreenY() - pan_delta;
				} else {
					return character->GetScreenY();
				}
			}
			case 6:
				// Event ID
				return Player::IsPatchManiac() ? interpreter.GetThisEventId() : 0;
		}

		Output::Warning("ControlVariables::Event: Unknown op {}", op);
	} else {
		Output::Warning("ControlVariables::Event: Bad event_id {}", event_id);
	}

	return 0;
}

template<bool ManiacPatch>
int ControlVariables::Other(int op) {
	switch (op) {
		case 0:
			// Gold
			return game_party->GetGold();
			break;
		case 1:
			// Timer 1 remaining time
			return game_party->GetTimerSeconds(game_party->Timer1);
			break;
		case 2:
			// Number of heroes in party
			return game_party->GetActors().size();
			break;
		case 3:
			// Number of saves
			return game_system->GetSaveCount();
			break;
		case 4:
			// Number of battles
			return game_party->GetBattleCount();
			break;
		case 5:
			// Number of wins
			return game_party->GetWinCount();
			break;
		case 6:
			// Number of defeats
			return game_party->GetDefeatCount();
			break;
		case 7:
			// Number of escapes (aka run away)
			return game_party->GetRunCount();
			break;
		case 8:
			// MIDI play position
			if (Player::IsPatchKeyPatch()) {
				return game_ineluki->GetMidiTicks();
			} else {
				return Audio().BGM_GetTicks();
			}
			break;
		case 9:
			// Timer 2 remaining time
			return game_party->GetTimerSeconds(game_party->Timer2);
			break;
		case 10:
			// Current date (YYMMDD)
			if constexpr (ManiacPatch) {
				std::time_t t = std::time(nullptr);
				std::tm* tm = std::localtime(&t);
				return atoi(Utils::FormatDate(tm, Utils::DateFormat_YYMMDD).c_str());
			}
			break;
		case 11:
			// Current time (HHMMSS)
			if constexpr (ManiacPatch) {
				std::time_t t = std::time(nullptr);
				std::tm* tm = std::localtime(&t);
				return atoi(Utils::FormatDate(tm, Utils::DateFormat_HHMMSS).c_str());
			}
			break;
		case 12:
			// Frames
			if constexpr (ManiacPatch) {
				return game_system->GetFrameCounter();
			}
			break;
		case 13:
			// Patch version
			if constexpr (ManiacPatch) {
				// Latest version before the engine rewrite
				return 200128;
			}
			break;
	}

	Output::Warning("ControlVariables::Other: Unknown op {}", op);
	return 0;
}

template <bool ManiacPatch>
int ControlVariables::Enemy(int op, int enemy_idx) {
	auto enemy = game_enemyparty->GetEnemy(enemy_idx);

	if (!enemy) {
		Output::Warning("ControlVariables::Enemy: Bad enemy_idx {}", enemy_idx);
		return 0;
	}

	switch (op) {
		case 0:
			// Enemy HP
			return enemy->GetHp();
		case 1:
			// Enemy SP
			return enemy->GetSp();
		case 2:
			// Enemy MaxHP
			return enemy->GetMaxHp();
		case 3:
			// Enemy MaxSP
			return enemy->GetMaxSp();
		case 4:
			// Enemy Attack
			return enemy->GetAtk();
		case 5:
			// Enemy Defense
			return enemy->GetDef();
		case 6:
			// Enemy Spirit
			return enemy->GetSpi();
		case 7:
			// Enemy Agility
			return enemy->GetAgi();
		case 8:
			// ID
			if constexpr (ManiacPatch) {
				return enemy->GetId();
			}
			break;
		case 9:
			// ATB
			if constexpr (ManiacPatch) {
				return enemy->GetAtbGauge();
			}
			break;
	}

	Output::Warning("ControlVariables::Enemy: Unknown op {}", op);
	return 0;
}

int ControlVariables::Pow(int arg1, int arg2) {
	return static_cast<int>(std::pow(arg1, arg2));
}

int ControlVariables::Sqrt(int arg, int mul) {
	// This is not how negative sqrt works, just following the implementation here
	int res = static_cast<int>(sqrt(abs(arg)) * mul);
	if (arg < 0) {
		res = -res;
	}
	return res;
}

int ControlVariables::Sin(int arg1, int arg2, int mul) {
	float res = static_cast<float>(arg1);
	if (arg2 != 0) {
		res /= static_cast<float>(arg2);
	}
	return static_cast<int>(std::sin(res * M_PI / 180.f) * mul);
}

int ControlVariables::Cos(int arg1, int arg2, int mul) {
	float res = static_cast<float>(arg1);
	if (arg2 != 0) {
		res /= static_cast<float>(arg2);
	}
	return static_cast<int>(std::cos(res * M_PI / 180.f) * mul);
}

int ControlVariables::Atan2(int arg1, int arg2, int mul) {
	return static_cast<int>(std::atan2(arg1, arg2) * 180.f / M_PI * mul);
}

int ControlVariables::Min(int arg1, int arg2) {
	return std::min(arg1, arg2);
}

int ControlVariables::Max(int arg1, int arg2) {
	return std::max(arg1, arg2);
}

int ControlVariables::Abs(int arg) {
	return abs(arg);
}

int ControlVariables::Binary(int op, int arg1, int arg2) {
	// 64 Bit for overflow protection
	int64_t result = 0;

	auto arg1_64 = static_cast<int64_t>(arg1);
	auto arg2_64 = static_cast<int64_t>(arg2);

	switch (op) {
		case 1:
			result = arg1_64 + arg2_64;
			break;
		case 2:
			result = arg1_64 - arg2_64;
			break;
		case 3:
			result = arg1_64 * arg2_64;
			break;
		case 4:
			if (arg2_64 != 0) {
				result = arg1_64 / arg2_64;
			} else {
				result = arg1_64;
			}
			break;
		case 5:
			if (arg2_64 != 0) {
				result = arg1_64 % arg2_64;
			} else {
				result = arg1_64;
			}
			break;
		case 6:
			result = arg1_64 | arg2_64;
			break;
		case 7:
			result = arg1_64 & arg2_64;
			break;
		case 8:
			result = arg1_64 ^ arg2_64;
			break;
		case 9:
			result = arg1_64 << arg2_64;
			break;
		case 10:
			result = arg1_64 >> arg2_64;
			break;
		default:
			Output::Warning("ControlVariables::Binary: Unknown op {}", op);
			return 0;
	}

	return static_cast<int>(Utils::Clamp<int64_t>(result, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()));
}

int ControlVariables::Clamp(int arg1, int arg2, int arg3) {
	return Utils::Clamp(arg1, arg2, arg3);
}

int ControlVariables::Muldiv(int arg1, int arg2, int arg3) {
	auto arg1_64 = static_cast<int64_t>(arg1);
	auto arg2_64 = static_cast<int64_t>(arg2);
	auto arg3_64 = static_cast<int64_t>(arg3);

	if (arg3_64 == 0) {
		arg3_64 = 1;
	}

	return static_cast<int>(arg1_64 * arg2_64 / arg3_64);
}

int ControlVariables::Divmul(int arg1, int arg2, int arg3) {
	auto arg1_64 = static_cast<int64_t>(arg1);
	auto arg2_d = static_cast<double>(arg2);
	auto arg3_64 = static_cast<int64_t>(arg3);

	if (arg2_d == 0) {
		arg2_d = 1.0;
	}

	return static_cast<int>(arg1_64 / arg2_d * arg3_64);
}

int ControlVariables::Between(int arg1, int arg2, int arg3) {
	return (arg1 >= arg2 && arg2 <= arg3) ? 0 : 1;
}

/* Definitions */
namespace ConditionalBranching {

	template <CondBranchCommandsType op_type>
	bool condition_DefaultCase(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const&) {
		dispatch_table_default_case_triggered = true;
		Output::Warning("{}: Branch {} unsupported", get_op_name(op_type), com.parameters[0]);
		return false;
	};

	bool Switch(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		return game_switches->Get(com.parameters[1]) == (com.parameters[2] == 0);
	}

	bool Variable(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int value1 = game_variables->Get(com.parameters[1]);
		int value2 = Game_Interpreter_Shared::ValueOrVariable(com.parameters[2], com.parameters[3], interpreter);
		return Game_Interpreter_Shared::CheckOperator(value1, value2, com.parameters[4]);
	}

	bool Timer(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int value1 = game_party->GetTimerSeconds(game_party->Timer1);
		int value2 = com.parameters[1];
		switch (com.parameters[2]) {
			case 0:
				return (value1 >= value2);
			case 1:
				return (value1 <= value2);
		}
		return false;
	}

	bool Gold(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		if (com.parameters[2] == 0) {
			// Greater than or equal
			return (game_party->GetGold() >= com.parameters[1]);
		} else {
			// Less than or equal
			return (game_party->GetGold() <= com.parameters[1]);
		}
	}

	bool Item(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		if (com.parameters[2] == 0) {
			// Having
			return game_party->GetItemCount(com.parameters[1])
				+ game_party->GetEquippedItemCount(com.parameters[1]) > 0;
		} else {
			// Not having
			return game_party->GetItemCount(com.parameters[1])
				+ game_party->GetEquippedItemCount(com.parameters[1]) == 0;
		}
	}

	bool Hero(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int actor_id = com.parameters[1];
		Game_Actor* actor = game_actors->GetActor(actor_id);

		if (!actor) {
			Output::Warning("ConditionalBranch: Invalid actor ID {}", actor_id);
			return false;
		}

		switch (com.parameters[2]) {
			case 0:
				// Is actor in party
				return game_party->IsActorInParty(actor_id);
			case 1:
				// Name
				return (actor->GetName() == com.string);
			case 2:
				// Higher or equal level
				return (actor->GetLevel() >= com.parameters[3]);
			case 3:
				// Higher or equal HP
				return (actor->GetHp() >= com.parameters[3]);
			case 4:
				// Is skill learned
				return (actor->IsSkillLearned(com.parameters[3]));
			case 5:
				// Equipped object
				return (
					(actor->GetShieldId() == com.parameters[3]) ||
					(actor->GetArmorId() == com.parameters[3]) ||
					(actor->GetHelmetId() == com.parameters[3]) ||
					(actor->GetAccessoryId() == com.parameters[3]) ||
					(actor->GetWeaponId() == com.parameters[3])
					);
			case 6:
				// Has state
				return (actor->HasState(com.parameters[3]));
		}
		return false;
	}

	bool CharOrientation(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		Game_Character* character = interpreter.GetCharacter(com.parameters[1]);
		if (character != NULL) {
			return character->GetFacing() == com.parameters[2];
		}
		return false;
	}

	bool VehicleInUse(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		Game_Vehicle::Type vehicle_id = (Game_Vehicle::Type)(com.parameters[1] + 1);
		Game_Vehicle* vehicle = Game_Map::GetVehicle(vehicle_id);

		if (!vehicle) {
			Output::Warning("ConditionalBranch: Invalid vehicle ID {}", static_cast<int>(vehicle_id));
			return true;
		}

		return vehicle->IsInUse();
	}

	bool TriggeredByDecisionKey(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		return interpreter.GetFrame().triggered_by_decision_key;
	}

	bool BgmLoopedOnce(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		return game_system->BgmPlayedOnce();
	}

	bool Timer2(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int value1 = game_party->GetTimerSeconds(game_party->Timer2);
		int value2 = com.parameters[1];
		switch (com.parameters[2]) {
			case 0:
				return (value1 >= value2);
			case 1:
				return (value1 <= value2);
		}
		return false;
	}

	bool Other(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		switch (com.parameters[1]) {
			case 0:
				// Any savestate available
				return FileFinder::HasSavegame();
			case 1:
				// Is Test Play mode?
				return Player::debug_flag;
			case 2:
				// Is ATB wait on?
				return game_system->GetAtbMode() == lcf::rpg::SaveSystem::AtbMode_atb_wait;
			case 3:
				// Is Fullscreen active?
				return DisplayUi->IsFullscreen();
		}
		return false;
	}

	bool ManiacsOther(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		switch (com.parameters[1]) {
			case 0:
				return game_system->IsLoadedThisFrame();
			case 1:
				// Joypad is active (We always read from Controller so simply report 'true')
#if defined(USE_JOYSTICK) && defined(SUPPORT_JOYSTICK)
				return true;
#else
				return false;
#endif
				break;
			case 2:
				// FIXME: Window has focus. Needs function exposed in DisplayUi
				// Assuming 'true' as Player usually suspends when loosing focus
				return true;
		}
		return false;
	}

	bool ManiacsSwitchIndirect(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		return game_switches->Get(game_variables->Get(com.parameters[1])) == (com.parameters[2] == 0);;
	}

	bool ManiacsVariableIndirect(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int value1 = game_variables->GetIndirect(com.parameters[1]);
		int value2 = Game_Interpreter_Shared::ValueOrVariable(com.parameters[2], com.parameters[3], interpreter);
		return Game_Interpreter_Shared::CheckOperator(value1, value2, com.parameters[4]);
	}

	bool ManiacsStringComparison(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		int modes[] = {
			(com.parameters[1]) & 15, //str_l mode: 0 = direct, 1 = indirect
			(com.parameters[1] >> 4) & 15, //str_r mode: 0 = literal, 1 = direct, 2 = indirect
		};

		int op = com.parameters[4] & 3;
		int ignoreCase = com.parameters[4] >> 8 & 1;

		std::string str_param = ToString(com.string);
		StringView str_l = game_strings->GetWithMode(str_param, modes[0] + 1, com.parameters[2], *game_variables);
		StringView str_r = game_strings->GetWithMode(str_param, modes[1], com.parameters[3], *game_variables);
		return ManiacPatch::CheckString(str_l, str_r, op, ignoreCase);
	}

	bool ManiacsExpression(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) {
		return ManiacPatch::ParseExpression(MakeSpan(com.parameters).subspan(6), interpreter);
	}
}

EP_ALWAYS_INLINE bool ConditionalBranching::dispatch_table_condition::Execute(lcf::rpg::EventCommand const& com, Game_BaseInterpreterContext const& interpreter) const {

	const std::byte operand = static_cast<std::byte>(com.parameters[0]);
	bool result = ops[std::to_integer<int>(operand)](com, interpreter);

	if (EP_UNLIKELY(dispatch_table_default_case_triggered)) {
		dispatch_table_default_case_triggered = false;
		return false;
	}

	return result;
}

template <ConditionalBranching::CondBranchCommandsType op_type>
ConditionalBranching::dispatch_table_condition& ConditionalBranching::BuildDispatchTable(const bool include2k3Commands, const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx) {

	static_assert(op_type >= eCondBranch_Default && op_type < eCondBranch_LAST);
	assert(tables[static_cast<int>(op_type)] == nullptr);

	std::bitset<16> patch_flags;
	patch_flags.set(1, include2k3Commands);
	patch_flags.set(2, includeManiacs_200128);
	patch_flags.set(3, includeManiacs24xxxx);
	patch_flags.set(4, includeEasyRpgEx);

	std::map<ConditionalBranch, condition_Func> ops;

	// Vanilla operands
	ops[eCondition_Switch] = &Switch;
	ops[eCondition_Variable] = &Variable;
	ops[eCondition_Timer] = &Timer;
	ops[eCondition_Gold] = &Gold;
	ops[eCondition_Item] = &Item;
	ops[eCondition_Hero] = &Hero;
	ops[eCondition_CharOrientation] = &CharOrientation;
	ops[eCondition_VehicleInUse] = &VehicleInUse;
	ops[eCondition_TriggeredByDecisionKey] = &TriggeredByDecisionKey;
	ops[eCondition_BgmLoopedOnce] = &BgmLoopedOnce;
	if (include2k3Commands) {
		ops[eCondition_2k3_Timer2] = &Timer2;
		ops[eCondition_2k3_Other] = &Other;
	}
	// end Vanilla operands

	if (includeManiacs_200128) {
		ops[eCondition_Maniacs_Other] = &ManiacsOther;
		ops[eCondition_Maniacs_SwitchIndirect] = &ManiacsSwitchIndirect;
		ops[eCondition_Maniacs_VariableIndirect] = &ManiacsVariableIndirect;
		ops[eCondition_Maniacs_StringComparison] = &ManiacsStringComparison;
		ops[eCondition_Maniacs_Expression] = &ManiacsExpression;
	}

	dispatch_table_condition* dispatch_table = new dispatch_table_condition((int)patch_flags.to_ulong(), ops, &condition_DefaultCase<op_type>);
	tables[static_cast<int>(op_type)] = dispatch_table;

	return *dispatch_table;
}

void ConditionalBranching::RebuildDispatchTables(const bool include2k3Commands, const bool includeManiacs_200128, const bool includeManiacs24xxxx, const bool includeEasyRpgEx) {

	std::bitset<16> bitset;
	bitset.set(1, include2k3Commands);
	bitset.set(2, includeManiacs_200128);
	bitset.set(3, includeManiacs24xxxx);
	bitset.set(4, includeEasyRpgEx);

	const int patch_flags_new = (int)bitset.to_ulong();

	if (tables[eCondBranch_Default] != nullptr && tables[eCondBranch_Default]->GetPatchFlags() != patch_flags_new) {
		delete tables[eCondBranch_Default];
		tables[eCondBranch_Default] = nullptr;

		BuildDispatchTable<eCondBranch_Default>(include2k3Commands, includeManiacs_200128, includeManiacs24xxxx, includeEasyRpgEx);
	}
}

#endif
