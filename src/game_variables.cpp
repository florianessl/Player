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
#include "game_variables.h"
#include "output.h"
#include <lcf/reader_util.h>
#include <lcf/data.h>
#include "utils.h"
#include "rand.h"
#include <cmath>

constexpr Game_Variables::Var_t Game_Variables::min_2k;
constexpr Game_Variables::Var_t Game_Variables::max_2k;
constexpr Game_Variables::Var_t Game_Variables::min_2k3;
constexpr Game_Variables::Var_t Game_Variables::max_2k3;

namespace {
	using Var_t = Game_Variables::Var_t;

	lcf::rpg::ScopedVariable* GetScopedVariable(DataScopeType scope, int id) {
		switch (scope) {
			case DataScopeType::eDataScope_Map:
				return lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_map_variables, id);
			case DataScopeType::eDataScope_MapEvent:
				return lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_self_variables, id);
		}
		return nullptr;
	}

	constexpr Var_t VarSet(Var_t o, Var_t n) {
		(void)o;
		return n;
	}

	constexpr Var_t VarAdd(Var_t l, Var_t r) {
		Var_t res = 0;

#ifdef _MSC_VER
		res = l + r;
		if (res < 0 && l > 0 && r > 0) {
			return std::numeric_limits<Var_t>::max();
		}
		else if (res > 0 && l < 0 && r < 0) {
			return std::numeric_limits<Var_t>::min();
		}
#else
		if (EP_UNLIKELY(__builtin_add_overflow(l, r, &res))) {
			if (l >= 0 && r >= 0) {
				return std::numeric_limits<Var_t>::max();
			}
			return std::numeric_limits<Var_t>::min();
		}
#endif

		return res;
	}

	constexpr Var_t VarSub(Var_t l, Var_t r) {
		Var_t res = 0;

#ifdef _MSC_VER
		res = l - r;
		if (res < 0 && l > 0 && r < 0) {
			return std::numeric_limits<Var_t>::max();
		}
		else if (res > 0 && l < 0 && r > 0) {
			return std::numeric_limits<Var_t>::min();
		}
#else
		if (EP_UNLIKELY(__builtin_sub_overflow(l, r, &res))) {
			if (r < 0) {
				return std::numeric_limits<Var_t>::max();
			}
			return std::numeric_limits<Var_t>::min();
		}
#endif

		return res;
	}

	constexpr Var_t VarMult(Var_t l, Var_t r) {
		Var_t res = 0;

#ifdef _MSC_VER
		res = l * r;
		if (l != 0 && res / l != r) {
			if ((l > 0 && r > 0) || (l < 0 && r < 0)) {
				return std::numeric_limits<Var_t>::max();
			}
			else {
				return std::numeric_limits<Var_t>::min();
			}
		}
#else
		if (EP_UNLIKELY(__builtin_mul_overflow(l, r, &res))) {
			if ((l > 0 && r > 0) || (l < 0 && r < 0)) {
				return std::numeric_limits<Var_t>::max();
			}
			return std::numeric_limits<Var_t>::min();
		}
#endif

		return res;
	}

	constexpr Var_t VarDiv(Var_t n, Var_t d) {
		return EP_LIKELY(d != 0) ? n / d : n;
	};

	constexpr Var_t VarMod(Var_t n, Var_t d) {
		return EP_LIKELY(d != 0) ? n % d : 0;
	};

	constexpr Var_t VarBitOr(Var_t n, Var_t d) {
		return n | d;
	};

	constexpr Var_t VarBitAnd(Var_t n, Var_t d) {
		return n & d;
	};

	constexpr Var_t VarBitXor(Var_t n, Var_t d) {
		return n ^ d;
	};

	constexpr Var_t VarBitShiftLeft(Var_t n, Var_t d) {
		return n << d;
	};

	constexpr Var_t VarBitShiftRight(Var_t n, Var_t d) {
		return n >> d;
	};

}

Game_Variables::Game_Variables(Var_t minval, Var_t maxval) : Game_VariablesBase(minval, maxval) {
}

StringView Game_Variables::GetName(int id, DataScopeType scope) const {

	if (DynamicScope::IsGlobalScope(scope) || DynamicScope::IsFrameScope(scope)) {
		const lcf::rpg::Variable* var;

		switch (scope) {
		case DataScopeType::eDataScope_Global:
			var = lcf::ReaderUtil::GetElement(lcf::Data::variables, id);
			break;
		case DataScopeType::eDataScope_Frame:
		case DataScopeType::eDataScope_Frame_CarryOnPush:
		case DataScopeType::eDataScope_Frame_CarryOnPop:
		case DataScopeType::eDataScope_Frame_CarryOnBoth:
			var = lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_frame_variables, id);
			break;
		}

		if (!var) {
			// No warning, is valid because the variable array resizes dynamic during runtime
			return {};
		} else {
			return var->name;
		}
	} else {
		const lcf::rpg::ScopedVariable* sv = GetScopedVariable(scope, id);
		if (sv != nullptr)
			return sv->name;
		return "";
	}
}

uint8_t Game_Variables::scopedInitFlags(DataScopeType scope, int id) const {
	assert(DynamicScope::IsMapScope(scope) || DynamicScope::IsMapEventScope(scope));

	const lcf::rpg::ScopedVariable* sv = GetScopedVariable(scope, id);

	uint32_t flags = 0;
	if (sv != nullptr) {
		if (sv->is_readonly)
			flags = flags | ScopedDataStorage_t::eFlag_ReadOnly;
		if (sv->auto_reset)
			flags = flags | ScopedDataStorage_t::eFlag_AutoReset;
		if (sv->default_value_defined)
			flags = flags | ScopedDataStorage_t::eFlag_DefaultValueDefined;
		if (DynamicScope::IsMapScope(scope) && sv->map_group_inherited_value)
			flags = flags | ScopedDataStorage_t::eFlag_MapGrpInheritedValue;
	}

	return flags;
}

Game_Variables::Var_t Game_Variables::scopedGetDefaultValue(DataScopeType scope, int id) const {
	assert(DynamicScope::IsVariableScope(scope));

	const lcf::rpg::ScopedVariable* sv = GetScopedVariable(scope, id);
	return sv != nullptr && sv->default_value_defined ? sv->default_value : false;
}

void Game_Variables::scopedGetDataFromSaveElement(lcf::rpg::SaveScopedVariableData element, DataScopeType& scope, int& id, Var_t& value, int& map_id, int& event_id, bool& reset_flag) const {
	scope = static_cast<DataScopeType>(element.scope);
	id = element.id;
	value = element.value;
	map_id = element.map_id;
	event_id = element.event_id;
	reset_flag = element.auto_reset;
}

lcf::rpg::SaveScopedVariableData Game_Variables::scopedCreateSaveElement(DataScopeType scope, int id, Var_t value, int map_id, int event_id, bool reset_flag) const {
	lcf::rpg::SaveScopedVariableData result = lcf::rpg::SaveScopedVariableData();
	result.scope = static_cast<int>(scope);
	result.id = id;
	result.value = value;
	result.map_id = map_id;
	result.event_id = event_id;
	result.auto_reset = reset_flag;
	return result;
}

const typename Game_VariablesBase::FrameStorage_const_t Game_VariablesBase::GetFrameStorageImpl(const lcf::rpg::SaveEventExecFrame* frame) const {
#ifndef SCOPEDVARS_LIBLCF_STUB
	return std::tie(frame->easyrpg_frame_variables, frame->easyrpg_frame_variables_carry_flags_in, frame->easyrpg_frame_variables_carry_flags_out);
#else
	static std::vector<int32_t> vec;
	static std::vector<uint32_t> carry_flags_in, carry_flags_out;
	return std::tie(vec, carry_flags_in, carry_flags_out);
#endif
}

typename Game_VariablesBase::FrameStorage_t Game_VariablesBase::GetFrameStorageForEditImpl(lcf::rpg::SaveEventExecFrame* frame) {
#ifndef SCOPEDVARS_LIBLCF_STUB
	return std::tie(frame->easyrpg_frame_variables, frame->easyrpg_frame_variables_carry_flags_in, frame->easyrpg_frame_variables_carry_flags_out);
#else
	static std::vector<int32_t> vec;
	static std::vector<uint32_t> carry_flags_in, carry_flags_out;
	return std::tie(vec, carry_flags_in, carry_flags_out);
#endif
}

template <DataScopeType S, typename F, typename... Args>
void Game_Variables::WriteRangeVariable(int first_id, const int last_id, const int var_id, F&& op, Args... args) {
	if (var_id >= first_id && var_id <= last_id) {
		auto value = Get<S>(var_id, args...);
		PerformRangeOperation<S>(first_id, var_id, value, std::forward<F>(op), args...);
		first_id = var_id + 1;
	}
	auto value = Get<S>(var_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, std::forward<F>(op), args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::Add(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarAdd, "+=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::Sub(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarSub, "-=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::Mult(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarMult, "*=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::Div(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarDiv, "/=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::Mod(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarMod, "%=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::BitOr(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarBitOr, "|=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::BitAnd(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarBitAnd, "&=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::BitXor(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarBitXor, "^=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::BitShiftLeft(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarBitShiftLeft, "<<=", args...);
}

SCOPED_IMPL
Game_Variables::Var_t Game_Variables::BitShiftRight(int variable_id, Var_t value, Args... args) {
	return PerformOperation<S>(variable_id, value, VarBitShiftRight, ">>=", args...);
}

SCOPED_IMPL
inline void Game_Variables::AddRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "+=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarAdd, args...);
};

SCOPED_IMPL
inline void Game_Variables::SubRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "-=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarSub, args...);
};

SCOPED_IMPL
inline void Game_Variables::MultRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "*=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarMult, args...);
};

SCOPED_IMPL
inline void Game_Variables::DivRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "/=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarDiv, args...);
};

SCOPED_IMPL
inline void Game_Variables::ModRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "%=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarMod, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitOrRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "|=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarBitOr, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitAndRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "&=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarBitAnd, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitXorRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "^=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarBitXor, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitShiftLeftRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, "<<=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarBitShiftLeft, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitShiftRightRange(int first_id, int last_id, Var_t value, Args... args) {
	ValidateRangeOp<S>(first_id, last_id, value, ">>=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, VarBitShiftRight, args...);
};

SCOPED_IMPL
inline void Game_Variables::SetRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarSet, args...);
};

SCOPED_IMPL
inline void Game_Variables::AddRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "+=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarAdd, args...);
};

SCOPED_IMPL
inline void Game_Variables::SubRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "-=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarSub, args...);
};

SCOPED_IMPL
inline void Game_Variables::MultRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "*=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarMult, args...);
};

SCOPED_IMPL
inline void Game_Variables::DivRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "/=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarDiv, args...);
};

SCOPED_IMPL
inline void Game_Variables::ModRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "%=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarMod, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitOrRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "|=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarBitOr, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitAndRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "&=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarBitAnd, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitXorRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "^=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarBitXor, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitShiftLeftRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, "<<=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarBitShiftLeft, args...);
};

SCOPED_IMPL
inline void Game_Variables::BitShiftRightRangeVariable(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarOp<S>(first_id, last_id, var_id, ">>=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	WriteRangeVariable<S>(first_id, last_id, var_id, VarBitShiftRight, args...);
};

SCOPED_IMPL
void Game_Variables::SetRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarSet, args...);
};

SCOPED_IMPL
void Game_Variables::AddRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "+=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarAdd, args...);
};

SCOPED_IMPL
void Game_Variables::SubRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "-=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarSub, args...);
};

SCOPED_IMPL
void Game_Variables::MultRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "*=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarMult, args...);
};

SCOPED_IMPL
void Game_Variables::DivRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "/=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarDiv, args...);
};

SCOPED_IMPL
void Game_Variables::ModRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "%=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarMod, args...);
};

SCOPED_IMPL
void Game_Variables::BitOrRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "|=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarBitOr, args...);
};

SCOPED_IMPL
void Game_Variables::BitAndRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "&=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarBitAnd, args...);
};

SCOPED_IMPL
void Game_Variables::BitXorRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "^=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarBitXor, args...);
};

SCOPED_IMPL
void Game_Variables::BitShiftLeftRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, "<<=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarBitShiftLeft, args...);
};

SCOPED_IMPL
void Game_Variables::BitShiftRightRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args) {
	ValidateRangeVarIndirectOp<S>(first_id, last_id, var_id, ">>=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [this, var_id]() { return Get(Get(var_id)); }, VarBitShiftRight, args...);
};

SCOPED_IMPL
void Game_Variables::SetRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarSet, args...);
};

SCOPED_IMPL
void Game_Variables::AddRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "+=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarAdd, args...);
};

SCOPED_IMPL
void Game_Variables::SubRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "-=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarSub, args...);
};

SCOPED_IMPL
void Game_Variables::MultRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "*=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarMult, args...);
};

SCOPED_IMPL
void Game_Variables::DivRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "/=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarDiv, args...);
};

SCOPED_IMPL
void Game_Variables::ModRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "%=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarMod, args...);
};

SCOPED_IMPL
void Game_Variables::BitOrRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "|=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarBitOr, args...);
};

SCOPED_IMPL
void Game_Variables::BitAndRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "&=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarBitAnd, args...);
};

SCOPED_IMPL
void Game_Variables::BitXorRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "^=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarBitXor, args...);
};

SCOPED_IMPL
void Game_Variables::BitShiftLeftRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, "<<=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarBitShiftLeft, args...);
};

SCOPED_IMPL
void Game_Variables::BitShiftRightRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args) {
	ValidateRangeRandomOp<S>(first_id, last_id, minval, maxval, ">>=", args...);
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, [minval, maxval]() { return Rand::GetRandomNumber(minval, maxval); }, VarBitShiftRight, args...);
};

void Game_Variables::EnumerateRange(int first_id, int last_id, Var_t value) {
	if (EP_UNLIKELY(ShouldWarn<DataScopeType::eDataScope_Global>(first_id, last_id))) {
		Output::Debug("Invalid write enumerate(var[{},{}])!", first_id, last_id);
		--_warnings;
	}
	PrepareRange(first_id, last_id);
	Var_t out_value = value;
	PerformRangeOperation(first_id, last_id, [&out_value]() { return out_value++; }, VarSet);
}

void Game_Variables::SortRange(int first_id, int last_id, bool asc) {
	if (EP_UNLIKELY(ShouldWarn<DataScopeType::eDataScope_Global>(first_id, last_id))) {
		Output::Debug("Invalid write sort(var[{},{}])!", first_id, last_id);
		--_warnings;
	}
	auto& vv = GetStorageForEdit();
	int i = std::max(1, first_id);
	if (i < last_id) {
		vv.prepare_iterate(i, last_id);

		auto sorter = [&](auto&& fn) {
			std::stable_sort(vv.begin() + i, vv.begin() + last_id + 1, fn);
		};
		if (asc) {
			sorter(std::less<>());
		} else {
			sorter(std::greater<>());
		}
	}
}

void Game_Variables::ShuffleRange(int first_id, int last_id) {
	if (EP_UNLIKELY(ShouldWarn<DataScopeType::eDataScope_Global>(first_id, last_id))) {
		Output::Debug("Invalid write shuffle(var[{},{}])!", first_id, last_id);
		--_warnings;
	}
	auto& vv = GetStorageForEdit();
	for (int i = std::max(1, first_id); i <= last_id; ++i) {
		int rnd_num = Rand::GetRandomNumber(first_id, last_id) - 1;
		std::swap(vv[i], vv[rnd_num]);
	}
}

template <typename F>
void Game_Variables::WriteArray(const int first_id_a, const int last_id_a, const int first_id_b, F&& op) {
	auto& vv = GetStorageForEdit();
	int out_b = std::max(1, first_id_b);
	for (int i = std::max(1, first_id_a); i <= last_id_a; ++i) {
		auto& v_a = vv[i];
		auto v_b = vv[out_b++];
		v_a = Utils::Clamp(op(v_a, v_b), GetMinValue(), GetMaxValue());
	}
}

void Game_Variables::SetArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] = var[{},{}]!");
	// Maniac Patch uses memcpy which is actually a memmove
	// This ensures overlapping areas are copied properly
	if (first_id_a < first_id_b) {
		WriteArray(first_id_a, last_id_a, first_id_b, VarSet);
	} else {
		auto& vv = GetStorageForEdit();
		const int steps = std::max(0, last_id_a - first_id_a + 1);
		int out_b = std::max(1, first_id_b + steps - 1);
		int out_a = std::max(1, last_id_a);
		for (int i = 0; i < steps; ++i) {
			auto& v_a = vv[out_a--];
			auto v_b = vv[out_b--];
			v_a = Utils::Clamp(VarSet(v_a, v_b), GetMinValue(), GetMaxValue());
		}
	}
}

template <typename... Args>
void Game_Variables::PrepareArray(const int first_id_a, const int last_id_a, const int first_id_b, const char* warn, Args... args) {
	const int last_id_b = first_id_b + last_id_a - first_id_a;
	if (EP_UNLIKELY(ShouldWarn<DataScopeType::eDataScope_Global>(first_id_a, last_id_a) || ShouldWarn<DataScopeType::eDataScope_Global>(first_id_b, last_id_b))) {
		Output::Debug(warn, first_id_a, last_id_a, first_id_b, last_id_b, args...);
		--_warnings;
	}
	auto& vv = GetStorageForEdit();
	vv.prepare(first_id_a, last_id_a);
	vv.prepare(first_id_b, last_id_b);
}

void Game_Variables::AddArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] += var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarAdd);
}

void Game_Variables::SubArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] -= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarSub);
}

void Game_Variables::MultArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] *= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarMult);
}

void Game_Variables::DivArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] /= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarDiv);
}

void Game_Variables::ModArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] %= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarMod);
}

void Game_Variables::BitOrArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] |= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarBitOr);
}

void Game_Variables::BitAndArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] &= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarBitAnd);
}

void Game_Variables::BitXorArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] ^= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarBitXor);
}

void Game_Variables::BitShiftLeftArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] <<= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarBitShiftLeft);
}

void Game_Variables::BitShiftRightArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] >>= var[{},{}]!");
	WriteArray(first_id_a, last_id_a, first_id_b, VarBitShiftRight);
}

void Game_Variables::SwapArray(int first_id_a, int last_id_a, int first_id_b) {
	PrepareArray(first_id_a, last_id_a, first_id_b, "Invalid write var[{},{}] <-> var[{},{}]!");
	auto& vv = GetStorageForEdit();
	const int steps = std::max(0, last_id_a - first_id_a + 1);
	int out_b = std::max(1, first_id_b + steps - 1);
	int out_a = std::max(1, last_id_a);
	for (int i = 0; i < steps; ++i) {
		std::swap(vv[out_a--], vv[out_b--]);
	}
}


int Game_Variables::GetMaxDigits() const {
	auto val = std::max(std::llabs(GetMaxValue()), std::llabs(GetMinValue()));
	return static_cast<int>(std::log10(val) + 1);
}



#define EXPLICIT_INSTANCES_OP(N) \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Global>(int variable_id, Var_t value); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Frame>(int variable_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int variable_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int variable_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int variable_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_Map>(int variable_id, Var_t value, int map_id); \
template Game_Variables::Var_t Game_Variables::N<DataScopeType::eDataScope_MapEvent>(int variable_id, Var_t value, int map_id, int event_id);

#define EXPLICIT_INSTANCES_RANGE_OP(N) \
template void Game_Variables::N<DataScopeType::eDataScope_Global>(int first_id, int last_id, Var_t value); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame>(int first_id, int last_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int first_id, int last_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int first_id, int last_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int first_id, int last_id, Var_t value, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Map>(int first_id, int last_id, Var_t value, int map_id); \
template void Game_Variables::N<DataScopeType::eDataScope_MapEvent>(int first_id, int last_id, Var_t value, int map_id, int event_id);

#define EXPLICIT_INSTANCES_RANGEVAR_OP(N) \
template void Game_Variables::N<DataScopeType::eDataScope_Global>(int first_id, int last_id, int var_id); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Map>(int first_id, int last_id, int var_id, int map_id); \
template void Game_Variables::N<DataScopeType::eDataScope_MapEvent>(int first_id, int last_id, int var_id, int map_id, int event_id);

#define EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(N) \
template void Game_Variables::N<DataScopeType::eDataScope_Global>(int first_id, int last_id, int var_id); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int first_id, int last_id, int var_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Map>(int first_id, int last_id, int var_id, int map_id); \
template void Game_Variables::N<DataScopeType::eDataScope_MapEvent>(int first_id, int last_id, int var_id, int map_id, int event_id);

#define EXPLICIT_INSTANCES_RANGE_RANDOM_OP(N) \
template void Game_Variables::N<DataScopeType::eDataScope_Global>(int first_id, int last_id, Var_t minval, Var_t maxval); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame>(int first_id, int last_id, Var_t minval, Var_t maxval, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int first_id, int last_id, Var_t minval, Var_t maxval, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int first_id, int last_id, Var_t minval, Var_t maxval, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int first_id, int last_id, Var_t minval, Var_t maxval, lcf::rpg::SaveEventExecFrame*); \
template void Game_Variables::N<DataScopeType::eDataScope_Map>(int first_id, int last_id, Var_t minval, Var_t maxval, int map_id); \
template void Game_Variables::N<DataScopeType::eDataScope_MapEvent>(int first_id, int last_id, Var_t minval, Var_t maxval, int map_id, int event_id);

EXPLICIT_INSTANCES_OP(Add)
EXPLICIT_INSTANCES_OP(Sub)
EXPLICIT_INSTANCES_OP(Mult)
EXPLICIT_INSTANCES_OP(Div)
EXPLICIT_INSTANCES_OP(Mod)
EXPLICIT_INSTANCES_OP(BitOr)
EXPLICIT_INSTANCES_OP(BitAnd)
EXPLICIT_INSTANCES_OP(BitXor)
EXPLICIT_INSTANCES_OP(BitShiftLeft)
EXPLICIT_INSTANCES_OP(BitShiftRight)

EXPLICIT_INSTANCES_RANGE_OP(AddRange)
EXPLICIT_INSTANCES_RANGE_OP(SubRange)
EXPLICIT_INSTANCES_RANGE_OP(MultRange)
EXPLICIT_INSTANCES_RANGE_OP(DivRange)
EXPLICIT_INSTANCES_RANGE_OP(ModRange)
EXPLICIT_INSTANCES_RANGE_OP(BitOrRange)
EXPLICIT_INSTANCES_RANGE_OP(BitAndRange)
EXPLICIT_INSTANCES_RANGE_OP(BitXorRange)
EXPLICIT_INSTANCES_RANGE_OP(BitShiftLeftRange)
EXPLICIT_INSTANCES_RANGE_OP(BitShiftRightRange)

EXPLICIT_INSTANCES_RANGEVAR_OP(SetRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(AddRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(SubRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(MultRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(DivRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(ModRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(BitOrRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(BitAndRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(BitXorRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(BitShiftLeftRangeVariable)
EXPLICIT_INSTANCES_RANGEVAR_OP(BitShiftRightRangeVariable)

EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(SetRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(AddRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(SubRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(MultRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(DivRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(ModRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(BitOrRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(BitAndRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(BitXorRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(BitShiftLeftRangeVariableIndirect)
EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP(BitShiftRightRangeVariableIndirect)

EXPLICIT_INSTANCES_RANGE_RANDOM_OP(SetRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(AddRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(SubRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(MultRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(DivRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(ModRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(BitOrRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(BitAndRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(BitXorRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(BitShiftLeftRangeRandom)
EXPLICIT_INSTANCES_RANGE_RANDOM_OP(BitShiftRightRangeRandom)

#undef EXPLICIT_INSTANCES_OP
#undef EXPLICIT_INSTANCES_RANGE_OP
#undef EXPLICIT_INSTANCES_RANGEVAR_OP
#undef EXPLICIT_INSTANCES_RANGEVARINDIRECT_OP
#undef EXPLICIT_INSTANCES_RANGE_RANDOM_OP
