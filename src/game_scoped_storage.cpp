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
#include "game_scoped_storage.h"
#include "output.h"
#include <lcf/reader_util.h>
#include <lcf/data.h>

Game_DataStorage_Declare_TPL
SCOPED_IMPL V Game_DataStorage_TPL::PerformOperation(const int id, const V value, op_Func&& op, const char* warnOp, Args... args) {

	if constexpr (DynamicScope::IsVariableScope(S) && !DynamicScope::IsFrameScope(S)) {
		if (!scopedValidateReadOnly<S>(id, id, args...))
			return scopedGetDefaultValue(S, id);
	}

	if constexpr (sizeof...(Args) == 0) {
		ASSERT_IS_GLOBAL_SCOPE(S, "SetOp");

		if (EP_UNLIKELY(ShouldWarn<S>(id, id))) {
			WarnSet<S>(id, value, warnOp, 0, 0);
		}

		if (id <= 0) {
			return _defaultValue;
		}
		auto& storage = this->template GetStorageForEdit<S>();
		storage.prepare(id, id);

		if constexpr (std::is_same<V, bool>::value) {
			bool v = storage.vector_ref()[id - 1];
			AssignOp(v, op(v, value));
			storage.vector_ref()[id - 1] = v;
			return v;
		} else {
			V& v = storage[id];
			AssignOp(v, op(v, value));
			return v;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		constexpr bool carry_in = (S == eDataScope_Frame_CarryOnPush || S == eDataScope_Frame_CarryOnBoth);
		constexpr bool carry_out = (S == eDataScope_Frame_CarryOnPop || S == eDataScope_Frame_CarryOnBoth);

		if (EP_UNLIKELY(scopedShouldWarn<S>(id, id, 0, 0))) {
			WarnSet<S>(id, value, warnOp, 0, 0);
		}

		if (id <= 0 || id > std::min(id, static_cast<int>(this->template GetLimit<S>()))) {
			return _defaultValue;
		}
		auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorageForEdit(args...);
		if (EP_UNLIKELY(id > static_cast<int>(vec.size()))) {
			vec.resize(id, false);
		}

		if constexpr (carry_in) {
			SetCarryFlagForFrameStorage(carry_flags_in, id);
		}
		if constexpr (carry_out) {
			SetCarryFlagForFrameStorage(carry_flags_out, id);
		}

		if constexpr (std::is_same<V, bool>::value) {
			bool v = vec[id - 1];
			AssignOp(v, op(v, value));
			vec[id - 1] = v;
			return v;
		} else {
			V& v = static_cast<V>(vec[id - 1]);
			AssignOp(v, op(v, value));
			return v;
		}
	} else {
		ASSERT_IS_VARIABLE_SCOPE(S, "SetOp");

		if (EP_UNLIKELY(scopedShouldWarn<S>(id, id, args...))) {
			WarnSet<S>(id, value, warnOp, args...);
		}

		if (id <= 0) {
			return scopedGetDefaultValue(S, id);
		}
		auto& storage = GetScopedStorageForEdit<S>(true, args...);
		PrepareRange<S>(id, id, args...);

		storage.flags[id - 1] |= ScopedDataStorage_t::eFlag_ValueDefined;

		if constexpr (std::is_same<V, bool>::value) {
			bool v = storage.vector_ref()[id - 1];
			AssignOp(v, op(v, value));
			storage.vector_ref()[id - 1] = v;
			return v;
		} else {
			V& v = storage[id];
			AssignOp(v, op(v, value));
			return v;
		}
	}
}

Game_DataStorage_Declare_TPL
SCOPED_IMPL inline V Game_DataStorage_TPL::Set(int id, V value, Args... args) {

	return PerformOperation<S>(id, std::forward<V>(value), [](V o, V n) {
		(void)o;
		return n;
	}, "=", args...);
}

Game_DataStorage_Declare_TPL
SCOPED_IMPL void Game_DataStorage_TPL::PrepareRange(const int first_id, const int last_id, Args... args) {

	if constexpr (DynamicScope::IsFrameScope(S)) {
		auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorageForEdit(args...);
		int hardcapped_last_id = std::min(last_id, static_cast<int>(this->template GetLimit<S>()));
		if (hardcapped_last_id > static_cast<int>(vec.size())) {
			vec.resize(hardcapped_last_id, false);
		}
	} else if constexpr (storage_mode == VarStorage::eStorageMode_Vector) {
		if constexpr (sizeof...(Args) == 0) {
			ASSERT_IS_GLOBAL_SCOPE(S, "PrepareRange");

			auto& storage = this->template GetStorageForEdit<S>();
			storage.prepare(first_id, last_id);
		} else  {
			ASSERT_IS_VARIABLE_SCOPE(S, "PrepareRange");

			auto& storage = this->template GetScopedStorageForEdit<S>(true, args...);

			int prepare_last_id = std::max(last_id, static_cast<int>(this->template GetLimit<S>())),
				prepare_first_id = std::min(prepare_last_id, static_cast<int>(storage.size() + 1));

			if (prepare_first_id >= prepare_last_id)
				return;

			storage.prepare(prepare_first_id, prepare_last_id);

			for (int i = prepare_first_id; i <= prepare_last_id; i++) {
				storage.flags[i - 1] = scopedInitFlags(S, i);
				storage.vector_ref()[i - 1] = scopedGetDefaultValue(S, i);
			}
		}
	}
}

Game_DataStorage_Declare_TPL
SCOPED_IMPL void Game_DataStorage_TPL::PerformRangeOperation(const int first_id, const int last_id, const V value, op_Func&& op, Args... args) {
	return PerformRangeOperation<S>(first_id, last_id, [&value]() { return value; }, std::forward<op_Func>(op), args...);
}

Game_DataStorage_Declare_TPL
SCOPED_IMPL void Game_DataStorage_TPL::SetRange(int first_id, int last_id, V value, Args... args) {
	if constexpr (DynamicScope::IsGlobalScope(S)) {
		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid write {} = {}!", this->FormatLValue<S>(first_id, last_id, args...), this->FormatRValue(value));
			--_warnings;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, 0, 0))) {
			Output::Debug("Invalid write {} = {}!", this->FormatLValue<S>(first_id, last_id), this->FormatRValue(value));
			--_warnings;
		}
	} else {
		if (!scopedValidateReadOnly<S>(first_id, last_id, args...))
			return;
		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid write {} = {}!", this->FormatLValue<S>(first_id, last_id, args...), this->FormatRValue(value));
			--_warnings;
		}
	}
	PrepareRange<S>(first_id, last_id, args...);
	PerformRangeOperation<S>(first_id, last_id, value, [](V o, V n) {
		(void)o;
		return n;
	}, args...);
}

Game_DataStorage_Declare_TPL
void Game_DataStorage_TPL::SetScopedStorageSaveData(std::vector<T> save) {
	_scopedData.clear();

	DataScopeType scope = DataScopeType::eDataScope_Global;
	int id = 0;
	V value;
	int map_id = 0;
	int event_id = 0;
	bool reset_flag = false;

	for (auto it = save.begin(); it != save.end();) {
		scopedGetDataFromSaveElement(*it, scope, id, value, map_id, event_id, reset_flag);

		if (scope == DataScopeType::eDataScope_Map) {
			Set<DataScopeType::eDataScope_Map>(id, value, map_id);
			if (reset_flag)
				scopedSetResetFlagForId<DataScopeType::eDataScope_Map>(id, true, map_id);
		} else if (scope == DataScopeType::eDataScope_MapEvent) {
			Set<DataScopeType::eDataScope_MapEvent>(id, value, map_id, event_id);
			if (reset_flag)
				scopedSetResetFlagForId<DataScopeType::eDataScope_MapEvent>(id, true, map_id, event_id);
		}
	}
}

Game_DataStorage_Declare_TPL
std::vector<T> Game_DataStorage_TPL::GetScopedStorageSaveData() const {
	std::vector<T> save;

	DataScopeType scope;
	int map_id = 0;
	int event_id = 0;	

	for (auto it = _scopedData.begin(); it != _scopedData.end();) {
		IdsFromHash(it->first, scope, map_id, event_id);

		if (!(scope == eDataScope_Map || scope == eDataScope_MapEvent))
			continue;

		auto& storage = (scope == eDataScope_Map) ? GetScopedStorage<eDataScope_Map>(map_id) : GetScopedStorage<eDataScope_MapEvent>(map_id, event_id);

		if (!storage.valid)
			continue;

		for (auto it2 = storage.flags.begin(); it2 != storage.flags.end();it++) {
			if ((it2->second & ScopedDataStorage_t::eFlag_ValueDefined) == 0)
				continue;
			T data = scopedCreateSaveElement(scope, it2->first, storage[it2->first + 1], map_id, event_id, (it2->second & ScopedDataStorage_t::eFlag_AutoReset) > 0);
			save.push_back(data);
		}
	}

	return save;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsStorageInitialized(int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsStorageInitialized");

	auto& storage = GetScopedStorage<S>(map_id, event_id);
	return storage.valid;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedGetInherited(int id, int map_id, parent_id_Func&& get_parent_id, V& out_val) const {
	ASSERT_IS_MAP_SCOPE(S, "scopedGetInherited");

	bool defined = scopedIsDefined<S>(id, map_id);
	if (!defined) {
		int parent_map_id = get_parent_id(map_id);
		if (map_id == parent_map_id) {
			Output::Error("Invalid parent lookup for {}!", FormatLValue<S>(id, id, map_id, 0));
			return false;
		}
		if (parent_map_id <= 0)
			return false;

		return scopedGetInherited<S>(id, parent_map_id, std::forward<parent_id_Func>(get_parent_id), out_val);
	}
	out_val = Get<S>(id, map_id);
	return true;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsDefined(int id, int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsDefined");

	auto& storage = GetScopedStorage<S>(map_id, event_id);
	if (!storage.valid)
		return false;
	auto it = storage.flags.find(id - 1);
	return it != storage.flags.end() && (ScopedDataStorage_t::eFlag_ValueDefined & it->second) > 0;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsReadOnly(int id, int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsReadOnly");

	auto& storage = GetScopedStorage<S>(map_id, event_id);
	if (!storage.valid) {
		auto flags = scopedInitFlags(S, id);
		return (ScopedDataStorage_t::eFlag_ReadOnly & flags) > 0;
	}
	auto it = storage.flags.find(id - 1);
	return it != storage.flags.end() && (ScopedDataStorage_t::eFlag_ReadOnly & it->second) > 0;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsDefaultValueDefined(int id, int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsDefaultValueDefined");

	auto& storage = GetScopedStorage<S>(map_id, event_id);
	if (!storage.valid) {
		auto flags = scopedInitFlags(S, id);
		return (ScopedDataStorage_t::eFlag_DefaultValueDefined & flags) > 0;
	}
	auto it = storage.flags.find(id - 1);
	return it != storage.flags.end() && (ScopedDataStorage_t::eFlag_DefaultValueDefined & it->second) > 0;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsAutoReset(int id, int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsAutoReset");

	auto& storage = GetScopedStorage<S>(map_id, event_id);
	if (!storage.valid) {
		auto flags = scopedInitFlags(S, id);
		return (ScopedDataStorage_t::eFlag_AutoReset & flags) > 0;
	}
	auto it = storage.flags.find(id - 1);
	return it != storage.flags.end() && (ScopedDataStorage_t::eFlag_AutoReset & it->second) > 0;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
bool Game_DataStorage_TPL::scopedIsInheritedValue(int id, int map_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedIsInheritedValue");

	auto flags = scopedInitFlags(S, id);
	return (ScopedDataStorage_t::eFlag_MapGrpInheritedValue & flags) > 0;
}

//TODO: needs to be tested
Game_DataStorage_Declare_TPL
template<DataScopeType S>
int Game_DataStorage_TPL::scopedCountElementsDefined(bool defined, int id, int map_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedCountElementsDefined");

	if (DynamicScope::IsMapScope(S) && map_id > 0)
		assert("scopedCountElementsDefined: param 'map_id' only valid for scope 'MapEvent'");

	int c = 0;

	for (auto it = _scopedData.begin(); it != _scopedData.end();it++) {
		if (!IsOfScope<S>(it->first))
			continue;

		ScopedDataStorage_t storage = it->second;

		if constexpr (S == DataScopeType::eDataScope_MapEvent) {
			if (map_id != storage.map_id)
				continue;
		}

		if ((storage.flags[id - 1] & ScopedDataStorage_t::eFlag_ValueDefined) == 0)
			continue;

		c++;
	}

	return c;
}

//TODO: needs to be tested
Game_DataStorage_Declare_TPL
template<DataScopeType scope>
int Game_DataStorage_TPL::scopedCountElementsWithCondition(std::function<bool(const V)> op, int id, int map_id) const {
	ASSERT_IS_VARIABLE_SCOPE(scope, "scopedCountElementsWithCondition");

	if (DynamicScope::IsMapScope(scope) && map_id > 0)
		assert("scopedCountElementsWithCondition: param 'map_id' only valid for scope 'MapEvent'");

	int c = 0;

	for (auto it = _scopedData.begin(); it != _scopedData.end();it++) {
		if (!IsOfScope<scope>(it->first))
			continue;

		ScopedDataStorage_t storage = it->second;

		if constexpr (scope == DataScopeType::eDataScope_MapEvent) {
			if (map_id != storage.map_id)
				continue;
		}
		V value = storage[id];
		if (op(value))
			c++;
	}

	return c;
}

//TODO: needs to be tested
Game_DataStorage_Declare_TPL
template<DataScopeType S>
std::vector<std::tuple<int, int>> Game_DataStorage_TPL::scopedGetElementsDefined(bool defined, int max, int id, int map_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedGetElementsDefined");

	if (DynamicScope::IsMapScope(S) && map_id > 0)
		assert("scopedGetElementsDefined: param 'map_id' only valid for scope 'MapEvent'");

	int c = 0;
	std::vector<std::tuple<int, int>> result;

	for (auto it = _scopedData.begin(); it != _scopedData.end() && c < max;it++) {
		if (!IsOfScope<S>(it->first))
			continue;

		ScopedDataStorage_t storage = it->second;

		if constexpr (S == DataScopeType::eDataScope_MapEvent) {
			if (map_id != storage.map_id)
				continue;
		}

		if ((storage.flags[id - 1] & ScopedDataStorage_t::eFlag_ValueDefined) == 0)
			continue;

		c++;
		result.emplace_back(std::make_tuple(storage.map_id, storage.evt_id));
	}

	return result;
}

//TODO: needs to be tested
Game_DataStorage_Declare_TPL
template<DataScopeType scope>
std::vector<std::tuple<int, int>> Game_DataStorage_TPL::scopedGetElementsWithCondition(std::function<bool(const V)> op, int max, int id, int map_id) const {
	ASSERT_IS_VARIABLE_SCOPE(scope, "scopedGetElementsWithCondition");

	if (DynamicScope::IsMapScope(scope) && map_id > 0)
		assert("scopedGetElementsWithCondition: param 'map_id' only valid for scope 'MapEvent'");

	int c = 0;
	std::vector<std::tuple<int, int>> result;

	for (auto it = _scopedData.begin(); it != _scopedData.end() && c < max;it++) {
		if (!IsOfScope<scope>(it->first))
			continue;

		ScopedDataStorage_t storage = it->second;

		if constexpr (scope == DataScopeType::eDataScope_MapEvent) {
			if (map_id != storage.map_id)
				continue;
		}
		if (op(storage[id])) {
			c++;
			result.emplace_back(std::make_tuple(storage.map_id, storage.evt_id));
		}
	}

	return result;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
void Game_DataStorage_TPL::scopedClearValue(int id, int map_id, int event_id) {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedClearValue");

	auto& storage = GetScopedStorageForEdit<S>(false, map_id, event_id);
	if (!storage.valid || storage.flags[id - 1] & ScopedDataStorage_t::eFlag_ValueDefined == 0) {
		return;
	}

	storage.flags[id - 1] &= ~ScopedDataStorage_t::eFlag_ValueDefined;
	storage.vector_ref()[id - 1] = _defaultValue;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
void Game_DataStorage_TPL::scopedResetTemporaryData(int map_id, int event_id) {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedResetTemporaryData");

	auto& storage = GetScopedStorageForEdit<S>(false, map_id, event_id);
	if (!storage.valid) {
		return;
	}

	for (auto it = storage.flags.begin(); it != storage.flags.end(); it++) {
		if ((it->second & ScopedDataStorage_t::eFlag_ValueDefined) == 0)
			continue;
		if ((it->second & ScopedDataStorage_t::eFlag_AutoReset) == 0)
			continue;
		storage.vector_ref()[it->first] = scopedGetDefaultValue(S, it->first);
	}
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
void Game_DataStorage_TPL::scopedSetResetFlagForId(int id, bool reset_flag, int map_id, int event_id) {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedSetResetFlagForId");

	auto& storage_check = GetScopedStorageForEdit<S>(false, map_id, event_id);
	if (!storage_check.valid) {
		auto flags = scopedInitFlags(S, id);
		if (reset_flag == ((ScopedDataStorage_t::eFlag_AutoReset & flags) > 0)) {
			return;
		}
	}
	auto& storage = GetScopedStorageForEdit<S>(true, map_id, event_id);
	PrepareRange<S>(id, id, map_id, event_id);
	if (reset_flag)
		storage.flags[id - 1] |= ScopedDataStorage_t::eFlag_AutoReset;
	else
		storage.flags[id - 1] &= ~ScopedDataStorage_t::eFlag_AutoReset;
}

#define COMMA ,

#define SCOPED_FUNC_EXPLICIT_IMPL_FUNC(R, D, T, V, mode, N, Args) \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Global>(Args); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Frame>(Args, lcf::rpg::SaveEventExecFrame*); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Frame_CarryOnPush>(Args, lcf::rpg::SaveEventExecFrame*); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Frame_CarryOnPop>(Args, lcf::rpg::SaveEventExecFrame*); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(Args, lcf::rpg::SaveEventExecFrame*); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Map>(Args, int); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_MapEvent>(Args, int, int);

#define VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC(R, D, T, V, mode, N, Args) \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Map>(Args); \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_MapEvent>(Args);

#define VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(R, D, T, V, mode, N, Args) \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Map>(Args) const; \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_MapEvent>(Args) const;

#define MAPSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(R, D, T, V, mode, N, Args) \
template R Game_DataStorage<D, T, V, mode>::N<DataScopeType::eDataScope_Map>(Args) const;


#define EXPLICIT_IMPL(D, T, V, mode) \
SCOPED_FUNC_EXPLICIT_IMPL_FUNC(V, D, T, V, mode, PerformOperation, int COMMA V COMMA op_Func&& COMMA const char*) \
SCOPED_FUNC_EXPLICIT_IMPL_FUNC(V, D, T, V, mode, Set, int COMMA V) \
SCOPED_FUNC_EXPLICIT_IMPL_FUNC(void, D, T, V, mode, SetRange, int COMMA int COMMA V) \
SCOPED_FUNC_EXPLICIT_IMPL_FUNC(void, D, T, V, mode, PrepareRange, const int COMMA const int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC(void, D, T, V, mode, scopedResetTemporaryData, int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC(void, D, T, V, mode, scopedSetResetFlagForId, int COMMA bool COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedIsStorageInitialized, int COMMA int) \
MAPSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedGetInherited, int COMMA int COMMA parent_id_Func&& COMMA V&) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedIsDefined, int COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedIsAutoReset, int COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedIsDefaultValueDefined, int COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(bool, D, T, V, mode, scopedIsInheritedValue, int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(int, D, T, V, mode, scopedCountElementsDefined, bool COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(int, D, T, V, mode, scopedCountElementsWithCondition, std::function<bool(const V)> op COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(std::vector<std::tuple<int COMMA int>>, D, T, V, mode, scopedGetElementsDefined, bool COMMA int COMMA int COMMA int) \
VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST(std::vector<std::tuple<int COMMA int>>, D, T, V, mode, scopedGetElementsWithCondition, std::function<bool(const V)> op COMMA int COMMA int COMMA int)

EXPLICIT_IMPL(Game_SwitchesBase, lcf::rpg::SaveScopedSwitchData, game_bool, VarStorage::eStorageMode_Vector);
EXPLICIT_IMPL(Game_VariablesBase, lcf::rpg::SaveScopedVariableData, int32_t, VarStorage::eStorageMode_Vector);

/* EXPLICIT_IMPL(lcf::rpg::SaveScopedStringData, std::string, VarStorage::eStorageMode_HashMap); */

#undef COMMA
#undef SCOPED_FUNC_EXPLICIT_IMPL_FUNC
#undef VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC
#undef VARSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST
#undef MAPSCOPED_FUNC_EXPLICIT_IMPL_FUNC_CONST
#undef EXPLICIT_IMPL
