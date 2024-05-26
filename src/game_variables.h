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

#ifndef EP_GAME_VARIABLES_H
#define EP_GAME_VARIABLES_H

// Headers
#include "game_scoped_storage.h"
#include "output.h"
#include <lcf/reader_util.h>
#include <lcf/data.h>
#include <lcf/rpg/database.h>
#include "utils.h"
#include "rand.h"
#include <cmath>
#include "string_view.h"
#include <cstdint>
#include <string>

/**
 * Game_Variables class
 */
class Game_Variables : public Game_VariablesBase {
public:
	static constexpr int max_warnings = 10;

	static constexpr Var_t min_2k = -999999;
	static constexpr Var_t max_2k = 999999;
	static constexpr Var_t min_2k3 = -9999999;
	static constexpr Var_t max_2k3 = 9999999;

	Game_Variables(Var_t minval, Var_t maxval);

	StringView GetName(int id, DataScopeType scope = DataScopeType::eDataScope_Global) const override;

	template<DataScopeType = DataScopeType::eDataScope_Global>
	Var_t GetIndirect(int variable_id) const;

	template<DataScopeType, DataScopeType indirect_scope>
	Var_t scopedGetIndirect(int variable_id, int map_id, int indirect_map_id, int event_id = 0, int indirect_event_id = 0) const;

	SCOPED_WITH_DEFAULT Var_t Add(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t Sub(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t Mult(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t Div(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t Mod(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t BitOr(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t BitAnd(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t BitXor(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t BitShiftLeft(int variable_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT Var_t BitShiftRight(int variable_id, Var_t value, Args... args);

	SCOPED_WITH_DEFAULT void AddRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void SubRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void MultRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void DivRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void ModRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void BitOrRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void BitAndRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void BitXorRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftLeftRange(int first_id, int last_id, Var_t value, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftRightRange(int first_id, int last_id, Var_t value, Args... args);

	SCOPED_WITH_DEFAULT void SetRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void AddRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void SubRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void MultRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void DivRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void ModRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitOrRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitAndRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitXorRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftLeftRangeVariable(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftRightRangeVariable(int first_id, int last_id, int var_id, Args... args);

	SCOPED_WITH_DEFAULT void SetRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void AddRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void SubRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void MultRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void DivRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void ModRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitOrRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitAndRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitXorRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftLeftRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftRightRangeVariableIndirect(int first_id, int last_id, int var_id, Args... args);

	SCOPED_WITH_DEFAULT void SetRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void AddRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void SubRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void MultRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void DivRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void ModRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void BitOrRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void BitAndRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void BitXorRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftLeftRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);
	SCOPED_WITH_DEFAULT void BitShiftRightRangeRandom(int first_id, int last_id, Var_t minval, Var_t maxval, Args... args);

	void EnumerateRange(int first_id, int last_id, Var_t value);
	void SortRange(int first_id, int last_id, bool asc);
	void ShuffleRange(int first_id, int last_id);

	void SetArray(int first_id_a, int last_id_a, int first_id_b);
	void AddArray(int first_id_a, int last_id_a, int first_id_b);
	void SubArray(int first_id_a, int last_id_a, int first_id_b);
	void MultArray(int first_id_a, int last_id_a, int first_id_b);
	void DivArray(int first_id_a, int last_id_a, int first_id_b);
	void ModArray(int first_id_a, int last_id_a, int first_id_b);
	void BitOrArray(int first_id_a, int last_id_a, int first_id_b);
	void BitAndArray(int first_id_a, int last_id_a, int first_id_b);
	void BitXorArray(int first_id_a, int last_id_a, int first_id_b);
	void BitShiftLeftArray(int first_id_a, int last_id_a, int first_id_b);
	void BitShiftRightArray(int first_id_a, int last_id_a, int first_id_b);
	void SwapArray(int first_id_a, int last_id_a, int first_id_b);

	int GetMaxDigits() const;

protected:
	void AssignOpImpl(Var_t& target, Var_t value) const;

	SCOPED_WITH_DEFAULT void ValidateRangeOp(int first_id, int last_id, Var_t value, const char* op, Args... args) const;
	SCOPED_WITH_DEFAULT void ValidateRangeVarOp(int first_id, int last_id, int var_id, const char* op, Args... args) const;
	SCOPED_WITH_DEFAULT void ValidateRangeVarIndirectOp(int first_id, int last_id, int var_id, const char* op, Args... args) const;
	SCOPED_WITH_DEFAULT void ValidateRangeRandomOp(int first_id, int last_id, Var_t minval, Var_t maxval, const char* op, Args... args) const;

	template<DataScopeType = DataScopeType::eDataScope_Global, typename F, typename... Args>
	void WriteRangeVariable(int first_id, const int last_id, const int var_id, F&& op, Args... args);

	template <typename... Args>
	void PrepareArray(const int first_id_a, const int last_id_a, const int first_id_b, const char* warn, Args... args);
	template <typename F>
	void WriteArray(const int first_id_a, const int last_id_a, const int first_id_b, F&& op);

	uint8_t scopedInitFlags(DataScopeType scope, int id) const override;
	Var_t scopedGetDefaultValue(DataScopeType scope, int id) const override;
	void scopedGetDataFromSaveElement(lcf::rpg::SaveScopedVariableData element, DataScopeType& scope, int& id, Var_t& value, int& map_id, int& event_id, bool& reset_flag) const override;
	lcf::rpg::SaveScopedVariableData scopedCreateSaveElement(DataScopeType scope, int id, Var_t value, int map_id, int event_id, bool reset_flag) const override;
};

inline void Game_VariablesBase::AssignOpImpl(Var_t& target, Var_t value) const {
	target = Utils::Clamp(value, GetMinValue(), GetMaxValue());
}

template<DataScopeType S>
inline Game_Variables::Var_t Game_Variables::GetIndirect(int variable_id) const {
	auto val_indirect = Get<S>(variable_id);
	return Get(static_cast<int>(val_indirect));
}

template<DataScopeType scope, DataScopeType indirect_scope>
inline Game_Variables::Var_t Game_Variables::scopedGetIndirect(int variable_id, int map_id, int indirect_map_id, int event_id, int indirect_event_id) const {
	if constexpr (DynamicScope::IsGlobalScope(indirect_scope)) {
		auto val_indirect = Get<indirect_scope>(variable_id);

		if constexpr (DynamicScope::IsGlobalScope(scope)) {
			return Get<scope>(static_cast<int>(val_indirect));
		}
		else {
			return Get<scope>(static_cast<int>(val_indirect), map_id, event_id);
		}
	}
	else {
		auto val_indirect = Get<indirect_scope>(variable_id, indirect_map_id, indirect_event_id);

		if constexpr (DynamicScope::IsGlobalScope(scope)) {
			return Get<scope>(static_cast<int>(val_indirect));
		}
		else {
			return Get<scope>(static_cast<int>(val_indirect), map_id, event_id);
		}
	}
}

SCOPED_IMPL inline void Game_Variables::ValidateRangeOp(int first_id, int last_id, Var_t value, const char* op, Args... args) const {
	if constexpr (DynamicScope::IsGlobalScope(S)) {
		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, args...), op, this->FormatRValue(value));
			--_warnings;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, 0, 0))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, 0, 0), op, this->FormatRValue(value));
			--_warnings;
		}
	} else {
		if (!scopedValidateReadOnly<S>(first_id, last_id, args...))
			return;
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, args...), op, this->FormatRValue(value));
			--_warnings;
		}
	}
}

SCOPED_IMPL inline void Game_Variables::ValidateRangeVarOp(int first_id, int last_id, int var_id, const char* op, Args... args) const {
	if constexpr (DynamicScope::IsGlobalScope(S)) {
		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, args...), op, this->FormatRValue(var_id, "var"));
			--_warnings;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, 0, 0))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, 0, 0), op, this->FormatRValue(var_id, "var"));
			--_warnings;
		}
	} else {
		if (!scopedValidateReadOnly<S>(first_id, last_id, args...))
			return;
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid write {} {} {}!", this->FormatLValue<S>(first_id, last_id, args...), op, this->FormatRValue(var_id, "var"));
			--_warnings;
		}
	}
}

SCOPED_IMPL inline void Game_Variables::ValidateRangeVarIndirectOp(int first_id, int last_id, int var_id, const char* op, Args... args) const {
	if constexpr (DynamicScope::IsGlobalScope(S)) {
		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid write {} {} var[var[{}]]!", this->FormatLValue<S>(first_id, last_id, args...), op, var_id);
			--_warnings;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, 0, 0))) {
			Output::Debug("Invalid write {} {} var[var[{}]]!", this->FormatLValue<S>(first_id, last_id, 0, 0), op, var_id);
			--_warnings;
		}
	} else {
		if (!scopedValidateReadOnly<S>(first_id, last_id, args...))
			return;
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid write {} {} var[var[{}]]!", this->FormatLValue<S>(first_id, last_id, args...), op, var_id);
			--_warnings;
		}
	}
}

SCOPED_IMPL inline void Game_Variables::ValidateRangeRandomOp(int first_id, int last_id, Var_t minval, Var_t maxval, const char* op, Args... args) const {
	if constexpr (DynamicScope::IsGlobalScope(S)) {
		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid write {} {} rand({},{})!", this->FormatLValue<S>(first_id, last_id, args...), op, minval, maxval);
			--_warnings;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, 0, 0))) {
			Output::Debug("Invalid write {} {} rand({},{})!", this->FormatLValue<S>(first_id, last_id, 0, 0), op, minval, maxval);
			--_warnings;
		}
	} else {
		if (!scopedValidateReadOnly<S>(first_id, last_id, args...))
			return;
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid write {} {} rand({},{})!", this->FormatLValue<S>(first_id, last_id, args...), op, minval, maxval);
			--_warnings;
		}
	}
}

#endif
