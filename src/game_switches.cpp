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
#include "game_switches.h"
#include "output.h"
#include <lcf/reader_util.h>
#include <lcf/data.h>

Game_Switches::Game_Switches()
	: Game_SwitchesBase() {

}

namespace {
	lcf::rpg::ScopedSwitch* GetScopedSwitch(DataScopeType scope, int id) {
		switch (scope) {
			case DataScopeType::eDataScope_Map:
				return lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_map_switches, id);
			case DataScopeType::eDataScope_MapEvent:
				return lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_self_switches, id);
		}
		return nullptr;
	}
}

StringView Game_Switches::GetName(int id, DataScopeType scope) const {
	if (DynamicScope::IsGlobalScope(scope) || DynamicScope::IsFrameScope(scope)) {
		const lcf::rpg::Switch* sw;

		switch (scope) {
		case DataScopeType::eDataScope_Global:
			sw = lcf::ReaderUtil::GetElement(lcf::Data::switches, id);
			break;
		case DataScopeType::eDataScope_Frame:
		case DataScopeType::eDataScope_Frame_CarryOnPush:
		case DataScopeType::eDataScope_Frame_CarryOnPop:
		case DataScopeType::eDataScope_Frame_CarryOnBoth:
			sw = lcf::ReaderUtil::GetElement(lcf::Data::easyrpg_frame_switches, id);
			break;
		}

		if (!sw) {
			// No warning, is valid because the switch array resizes dynamic during runtime
			return {};
		} else {
			return sw->name;
		}
	} else {
		const lcf::rpg::ScopedSwitch* ssw = GetScopedSwitch(scope, id);
		if (ssw != nullptr)
			return ssw->name;
		return {};
	}
}

uint8_t Game_Switches::scopedInitFlags(DataScopeType scope, int id) const {
	assert(DynamicScope::IsMapScope(scope) || DynamicScope::IsMapEventScope(scope));

	const lcf::rpg::ScopedSwitch* ssw = GetScopedSwitch(scope, id);

	uint32_t flags = 0;
	if (ssw != nullptr) {
		if (ssw->is_readonly)
			flags = flags | ScopedDataStorage_t::eFlag_ReadOnly;
		if (ssw->auto_reset)
			flags = flags | ScopedDataStorage_t::eFlag_AutoReset;
		if (ssw->default_value_defined)
			flags = flags | ScopedDataStorage_t::eFlag_DefaultValueDefined;
		if (DynamicScope::IsMapScope(scope) && ssw->map_group_inherited_value)
			flags = flags | ScopedDataStorage_t::eFlag_MapGrpInheritedValue;
	}

	return flags;
}

game_bool Game_Switches::scopedGetDefaultValue(DataScopeType scope, int id) const {
	assert(DynamicScope::IsMapScope(scope) || DynamicScope::IsMapEventScope(scope));

	const lcf::rpg::ScopedSwitch* ssw = GetScopedSwitch(scope, id);
	return ssw != nullptr && ssw->default_value_defined ? ssw->default_value : false;
}

void Game_Switches::scopedGetDataFromSaveElement(lcf::rpg::SaveScopedSwitchData element, DataScopeType& scope, int& id, game_bool& value, int& map_id, int& event_id, bool& reset_flag) const {
	scope = static_cast<DataScopeType>(element.scope);
	id = element.id;
	value = element.on;
	map_id = element.map_id;
	event_id = element.event_id;
	reset_flag = element.auto_reset;
}

lcf::rpg::SaveScopedSwitchData Game_Switches::scopedCreateSaveElement(DataScopeType scope, int id, game_bool value, int map_id, int event_id, bool reset_flag) const {
	lcf::rpg::SaveScopedSwitchData result = lcf::rpg::SaveScopedSwitchData();
	result.scope = static_cast<int>(scope);
	result.id = id;
	result.on = value;
	result.map_id = map_id;
	result.event_id = event_id;
	result.auto_reset = reset_flag;
	return result;
}

typename const Game_SwitchesBase::FrameStorage_const_t Game_SwitchesBase::GetFrameStorageImpl(const lcf::rpg::SaveEventExecFrame* frame) const {
#ifndef SCOPEDVARS_LIBLCF_STUB
	return std::tie(frame->easyrpg_frame_switches, frame->easyrpg_frame_switches_carry_flags_in, frame->easyrpg_frame_switches_carry_flags_out);
#else
	static std::vector<game_bool> vec;
	static std::vector<uint32_t> carry_flags_in, carry_flags_out;
	return std::tie(vec, carry_flags_in, carry_flags_out);
#endif
}

typename Game_SwitchesBase::FrameStorage_t Game_SwitchesBase::GetFrameStorageForEditImpl(lcf::rpg::SaveEventExecFrame* frame) {
#ifndef SCOPEDVARS_LIBLCF_STUB
	return std::tie(frame->easyrpg_frame_switches, frame->easyrpg_frame_switches_carry_flags_in, frame->easyrpg_frame_switches_carry_flags_out);
#else
	static std::vector<game_bool> vec;
	static std::vector<uint32_t> carry_flags_in, carry_flags_out;
	return std::tie(vec, carry_flags_in, carry_flags_out);
#endif
}

SCOPED_IMPL game_bool Game_Switches::Flip(int id, Args... args) {

	if constexpr (sizeof...(Args) == 0) {
		ASSERT_IS_GLOBAL_SCOPE(S, "Flip");

		if (EP_UNLIKELY(ShouldWarn<S>(id, id))) {
			Output::Debug("Invalid flip {}!", FormatLValue<S>(id, 0, args...));
			--_warnings;
		}

		if (id <= 0) {
			return false;
		}
		auto& storage = GetStorageForEdit();
		storage.prepare(id, id);

		if constexpr (std::is_same<game_bool, bool>::value) {
			storage[id].flip();
			return (bool)storage[id];
		} else {
			game_bool& b = storage[id];
			b = (b > 0) ? 0 : 1;
			return b;
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		constexpr bool carry_in = (S == eDataScope_Frame_CarryOnPush || S == eDataScope_Frame_CarryOnBoth);
		constexpr bool carry_out = (S == eDataScope_Frame_CarryOnPop || S == eDataScope_Frame_CarryOnBoth);

		auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorageForEdit(args...);
		if (id > GetLimit<S>()) {
			return false;
		}
		PrepareRange<S>(id, id, args...);
		if constexpr (carry_in) {
			SetCarryFlagForFrameStorage(carry_flags_in, id);
		}
		if constexpr (carry_out) {
			SetCarryFlagForFrameStorage(carry_flags_out, id);
		}

		if constexpr (std::is_same<game_bool, bool>::value) {
			vec[id - 1].flip();
			return vec[id - 1];
		} else {
			game_bool& b = vec[id - 1];
			b = (b > 0) ? 0 : 1;
			return b;
		}
	} else {
		ASSERT_IS_VARIABLE_SCOPE(S, "Flip");

		if (EP_UNLIKELY(scopedShouldWarn<S>(id, id, args...))) {
			Output::Debug("Invalid flip {}!", FormatLValue<S>(id, 0, args...));
			--_warnings;
		}

		if (id <= 0) {
			return false;
		}
		auto& storage = GetScopedStorageForEdit<S>(true, args...);
		PrepareRange<S>(id, id, args...);

		storage.flags[id - 1] |= ScopedDataStorage_t::eFlag_ValueDefined;

		if constexpr (std::is_same<game_bool, bool>::value) {
			storage[id].flip();
			return (bool)storage[id];
		} else {
			game_bool& b = storage[id];
			b = (b > 0) ? 0 : 1;
			return b;
		}
	}
}

SCOPED_IMPL void Game_Switches::FlipRange(int first_id, int last_id, Args... args) {

	if constexpr (sizeof...(Args) == 0) {
		ASSERT_IS_GLOBAL_SCOPE(S, "FlipRange");

		if (EP_UNLIKELY(ShouldWarn<S>(first_id, last_id))) {
			Output::Debug("Invalid flip {}!", FormatLValue<S>(first_id, last_id, args...));
			--_warnings;
		}

		auto& storage = GetStorageForEdit();
		storage.prepare(first_id, last_id);

		for (int i = std::max(1, first_id); i <= last_id; ++i) {
			if constexpr (std::is_same<game_bool, bool>::value) {
				storage[i].flip();
			} else {
				game_bool& b = storage[i];
				b = (b > 0) ? 0 : 1;
			}
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		constexpr bool carry_in = (S == eDataScope_Frame_CarryOnPush || S == eDataScope_Frame_CarryOnBoth);
		constexpr bool carry_out = (S == eDataScope_Frame_CarryOnPop || S == eDataScope_Frame_CarryOnBoth);

		auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorageForEdit(args...);
		PrepareRange<S>(first_id, last_id, args...);
		int hardcapped_last_id = std::min(last_id, static_cast<int>(GetLimit<S>()));

		for (int i = std::max(0, first_id - 1); i < hardcapped_last_id; ++i) {
			if constexpr (carry_in) {
				SetCarryFlagForFrameStorage(carry_flags_in, i + 1);
			}
			if constexpr (carry_out) {
				SetCarryFlagForFrameStorage(carry_flags_out, i + 1);
			}

			if constexpr (std::is_same<game_bool, bool>::value) {
				vec[i].flip();
			} else {
				game_bool& b = vec[i];
				b = (b > 0) ? 0 : 1;
			}
		}
	} else {
		ASSERT_IS_VARIABLE_SCOPE(S, "FlipRange");

		if (EP_UNLIKELY(scopedShouldWarn<S>(first_id, last_id, args...))) {
			Output::Debug("Invalid flip {}!", FormatLValue<S>(first_id, last_id, args...));
			--_warnings;
		}

		auto& storage = GetScopedStorageForEdit<S>(true, args...);
		PrepareRange<S>(first_id, last_id, args...);

		for (int i = std::max(1, first_id); i <= last_id; ++i) {
			storage.flags[i - 1] |= ScopedDataStorage_t::eFlag_ValueDefined;

			if constexpr (std::is_same<game_bool, bool>::value) {
				storage[i].flip();
			} else {
				game_bool& b = storage[i];
				b = (b > 0) ? 0 : 1;
			}
		}
	}
}

#define EXPLICIT_INSTANCES_OP_CONST(R, N) \
template R Game_Switches::N<DataScopeType::eDataScope_Global>(int id) const; \
template R Game_Switches::N<DataScopeType::eDataScope_Frame>(int id, lcf::rpg::SaveEventExecFrame*) const; \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int id, lcf::rpg::SaveEventExecFrame*) const; \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int id, lcf::rpg::SaveEventExecFrame*) const; \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int id, lcf::rpg::SaveEventExecFrame*) const; \
template R Game_Switches::N<DataScopeType::eDataScope_Map>(int id, int map_id) const; \
template R Game_Switches::N<DataScopeType::eDataScope_MapEvent>(int id, int map_id, int event_id) const;

#define EXPLICIT_INSTANCES_OP(R, N) \
template R Game_Switches::N<DataScopeType::eDataScope_Global>(int id); \
template R Game_Switches::N<DataScopeType::eDataScope_Frame>(int id, lcf::rpg::SaveEventExecFrame*); \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int id, lcf::rpg::SaveEventExecFrame*); \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int id, lcf::rpg::SaveEventExecFrame*); \
template R Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int id, lcf::rpg::SaveEventExecFrame*); \
template R Game_Switches::N<DataScopeType::eDataScope_Map>(int id, int map_id); \
template R Game_Switches::N<DataScopeType::eDataScope_MapEvent>(int id, int map_id, int event_id);

#define EXPLICIT_INSTANCES_RANGE_OP(N) \
template void Game_Switches::N<DataScopeType::eDataScope_Global>(int first_id, int last_id); \
template void Game_Switches::N<DataScopeType::eDataScope_Frame>(int first_id, int last_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPush>(int first_id, int last_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnPop>(int first_id, int last_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Switches::N<DataScopeType::eDataScope_Frame_CarryOnBoth>(int first_id, int last_id, lcf::rpg::SaveEventExecFrame*); \
template void Game_Switches::N<DataScopeType::eDataScope_Map>(int first_id, int last_id, int map_id); \
template void Game_Switches::N<DataScopeType::eDataScope_MapEvent>(int first_id, int last_id, int map_id, int event_id);

EXPLICIT_INSTANCES_OP_CONST(int, GetInt)
EXPLICIT_INSTANCES_OP(game_bool, Flip)
EXPLICIT_INSTANCES_RANGE_OP(FlipRange)

#undef EXPLICIT_INSTANCES_OP_CONST
#undef EXPLICIT_INSTANCES_OP
#undef EXPLICIT_INSTANCES_RANGE_OP
