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

#include "game_interpreter_debug.h"
#include "game_interpreter.h"
#include "game_battle.h"
#include "game_map.h"
#include "main_data.h"
#include "game_variables.h"
#include "output.h"
#include <lcf/reader_util.h>

Debug::ParallelInterpreterStates Debug::ParallelInterpreterStates::GetCachedStates() {
	std::vector<int> ev_ids;
	std::vector<int> ce_ids;

	std::vector<lcf::rpg::SaveEventExecState> state_ev;
	std::vector<lcf::rpg::SaveEventExecState> state_ce;

	if (Game_Map::GetMapId() > 0) {
		for (auto& ev : Game_Map::GetEvents()) {
			if (ev.GetTrigger() != lcf::rpg::EventPage::Trigger_parallel || !ev.interpreter)
				continue;
			ev_ids.emplace_back(ev.GetId());
			state_ev.emplace_back(ev.interpreter->GetState());
		}
		for (auto& ce : Game_Map::GetCommonEvents()) {
			if (ce.IsWaitingBackgroundExecution(false)) {
				ce_ids.emplace_back(ce.common_event_id);
				state_ce.emplace_back(ce.interpreter->GetState());
			}
		}
	} else if (Game_Battle::IsBattleRunning() && Player::IsPatchManiac()) {
		//FIXME: Not implemented: battle common events
	}

	return { ev_ids, ce_ids, state_ev, state_ce };
}

std::vector<Debug::CallStackItem> Debug::CreateCallStack(const int owner_evt_id, const lcf::rpg::SaveEventExecState& state) {
	std::vector<CallStackItem> items;

	for (int i = state.stack.size() - 1; i >= 0; i--) {
		int evt_id = state.stack[i].event_id;
		int page_id = 0;
		if (state.stack[i].maniac_event_id > 0) {
			evt_id = state.stack[i].maniac_event_id;
			page_id = state.stack[i].maniac_event_page_id;
		}
		if (evt_id == 0 && i == 0)
			evt_id = owner_evt_id;

		auto item = Debug::CallStackItem();
		item.stack_item_no = i + 1;
		item.is_ce = (state.stack[i].easyrpg_runtime_flags & lcf::rpg::SaveEventExecFrame::RuntimeFlags_common_event) > 0;
		item.evt_id = evt_id;
		item.page_id = page_id;
		item.name = "";
		item.cmd_current = state.stack[i].current_command;
		item.cmd_count = state.stack[i].commands.size();

		if (item.is_ce) {
			auto* ce = lcf::ReaderUtil::GetElement(lcf::Data::commonevents, item.evt_id);
			if (ce) {
				item.name = ToString(ce->name);
			}
		} else {
			auto* ev = Game_Map::GetEvent(evt_id);
			if (ev) {
				//FIXME: map could have changed, but map_id isn't available
				item.name = ToString(ev->GetName());
			}
		}

		items.push_back(item);
	}

	return items;
}

std::string Debug::FormatEventName(Game_Character const& ch) {
	switch (ch.GetType()) {
		case Game_Character::Player:
			return "Player";
		case Game_Character::Vehicle:
		{
			int type = static_cast<Game_Vehicle const&>(ch).GetVehicleType();
			assert(type > Game_Vehicle::None && type <= Game_Vehicle::Airship);
			return Game_Vehicle::TypeNames[type];
		}
		case Game_Character::Event:
		{
			auto& ev = static_cast<Game_Event const&>(ch);
			if (ev.GetName().empty()) {
				return fmt::format("EV{:04d}", ev.GetId());
			}
			return fmt::format("EV{:04d} '{}'", ev.GetId(), ev.GetName());
		}
		default:
			assert(false);
	}
	return "";
}

std::string Debug::FormatEventName(Game_CommonEvent const& ev) {
	if (ev.GetName().empty()) {
		return fmt::format("CE{:04d}", ev.GetIndex());
	}
	return fmt::format("CE{:04d} '{}'", ev.GetIndex(), ev.GetName());
}

std::string Debug::FormatEventName(lcf::rpg::SaveEventExecFrame const* frame) {
	if (frame == nullptr) {
		return "Event";
	}
	if ((frame->easyrpg_runtime_flags & lcf::rpg::SaveEventExecFrame::RuntimeFlags_map_event) > 0) {
		return fmt::format("EV{:04d}", frame->maniac_event_id);
	}
	if ((frame->easyrpg_runtime_flags & lcf::rpg::SaveEventExecFrame::RuntimeFlags_common_event) > 0) {
		return fmt::format("CE{:04d}", frame->maniac_event_id);
	}
	if ((frame->easyrpg_runtime_flags & lcf::rpg::SaveEventExecFrame::RuntimeFlags_battle_event) > 0) {
		return fmt::format("BattlePage {}", frame->maniac_event_page_id);
	}
	return "Event";
}

void Debug::AssertBlockedMoves() {
	auto check = [](Game_Character& ev) {
		return ev.IsMoveRouteOverwritten() && !ev.IsMoveRouteFinished()
			&& ev.GetStopCount() != 0xFFFF && ev.GetStopCount() > ev.GetMaxStopCount();
	};
	auto assert_way = [](Game_Character& ev) {
		using Code = lcf::rpg::MoveCommand::Code;
		auto& move_command = ev.GetMoveRoute().move_commands[ev.GetMoveRouteIndex()];

		if (move_command.command_id >= static_cast<int>(Code::move_up)
			&& move_command.command_id <= static_cast<int>(Code::move_forward)) {

			const int dir = ev.GetDirection();
			const int from_x = ev.GetX(),
				from_y = ev.GetY(),
				to_x = from_x + ev.GetDxFromDirection(dir),
				to_y = from_y + ev.GetDyFromDirection(dir);

			if (from_x != to_x && from_y != to_y) {
				bool valid = Game_Map::AssertWay(ev, from_x, from_y, from_x, to_y);
				if (valid)
					valid = Game_Map::AssertWay(ev, from_x, to_y, to_x, to_y);
				if (valid)
					valid = Game_Map::AssertWay(ev, from_x, from_y, to_x, from_y);
				if (valid)
					valid = Game_Map::AssertWay(ev, to_x, from_y, to_x, to_y);
			} else {
				Game_Map::AssertWay(ev, from_x, from_y, to_x, to_y);
			}
		}
	};
	const auto map_id = Game_Map::GetMapId();
	if (auto& ch = *Main_Data::game_player; check(ch)) {
		assert_way(ch);
	}
	if (auto& vh = *Game_Map::GetVehicle(Game_Vehicle::Boat); vh.GetMapId() == map_id && check(vh)) {
		assert_way(vh);
	}
	if (auto& vh = *Game_Map::GetVehicle(Game_Vehicle::Ship); vh.GetMapId() == map_id && check(vh)) {
		assert_way(vh);
	}
	if (auto& vh = *Game_Map::GetVehicle(Game_Vehicle::Airship); vh.GetMapId() == map_id && check(vh)) {
		assert_way(vh);
	}
	for (auto& ev : Game_Map::GetEvents()) {
		if (check(ev)) {
			assert_way(ev);
		}
	}
}

#ifdef INTERPRETER_DEBUGGING

namespace Debug {
	Game_DebuggableInterpreterContext* active_interpreter = nullptr;
	bool in_execute_command = false;
	bool is_main_halted = false;
}

void Debug::LogCallback(LogLevel lvl, std::string const& msg, LogCallbackUserData /* userdata */) {
	if (lvl == LogLevel::Warning && active_interpreter) {
		if (active_interpreter->CanHaltExecution()) {
			active_interpreter->HaltExecution();
		}

		auto& state = static_cast<Game_Interpreter*>(active_interpreter)->GetState();
		if (state.stack.size() > 0 && (state.easyrpg_debug_flags & lcf::rpg::SaveEventExecState::DebugFlags_log_callstack_on_warnings)) {
			auto callstack = CreateCallStack(state.stack[0].event_id, state);

			auto& recent_frame = state.stack[state.stack.size() - 1];
			if (in_execute_command) {
				Output::Info("Callstack (stopped at command: '{}'; most recent frame first):",
					lcf::rpg::EventCommand::kCodeTags.tag(recent_frame.commands[recent_frame.current_command].code));
			} else if (recent_frame.current_command - 1 >= 0 && recent_frame.current_command < recent_frame.commands.size()) {
				if (state.wait_movement) {
					Output::Info("Callstack (waiting for movement):");
				} else {
					Output::Info("Callstack (stopped after command: '{}'; most recent frame first):",
						lcf::rpg::EventCommand::kCodeTags.tag(recent_frame.commands[recent_frame.current_command - 1].code));
				}
			}
			for (auto it = callstack.begin(); it < callstack.end(); ++it) {
				auto& item = *it;
				std::string evt_description;

				if (item.is_ce) {
					evt_description = fmt::format("CE{:04d}", item.evt_id);
				} else if (item.page_id > 0) {
					evt_description = fmt::format("EV{:04d}[{:02d}]", item.evt_id, item.page_id);
				} else {
					evt_description = fmt::format("EV{:04d}", item.evt_id);
				}

				if (!item.name.empty()) {
					evt_description.append("(\"");
					evt_description.append(item.name);
					evt_description.append("\")");
				}

				Output::Info(" [{:02d}] {} {:0d}/{:0d}", item.stack_item_no, evt_description, item.cmd_current, item.cmd_count);
			}
		}
	}
}

namespace Debug {

	template<typename T, int S>
	constexpr Span<T> to_span(std::array<T, S> array) {
		return Span<T>(array.data(), S);
	}

	void check_and_apply_flag(lcf::rpg::EventCommand const& com, lcf::rpg::SaveEventExecFrame::DebugFlags flag, Span<Cmd> data, uint32_t& debug_flags) {
		if ((debug_flags & static_cast<uint32_t>(flag)) > 0)
			return;
		for (size_t i = 0; i < data.size(); ++i) {
			if (static_cast<Cmd>(com.code) == data[i]) {
				debug_flags |= static_cast<uint32_t>(flag);
				return;
			}
		}
	}

	void check_and_apply_flag_tuples(lcf::rpg::EventCommand const& com, lcf::rpg::SaveEventExecFrame::DebugFlags flag, Span <std::tuple<Cmd, int>> data, uint32_t& debug_flags) {
		if ((debug_flags & static_cast<uint32_t>(flag)) > 0)
			return;
		for (size_t i = 0; i < data.size(); ++i) {
			if (static_cast<Cmd>(com.code) == std::get<0>(data[i])) {
				int com_param = std::get<1>(data[i]);
				if (com_param < 0 || com.parameters.size() <= com_param || com.parameters[com_param] != 0) {
					debug_flags |= static_cast<uint32_t>(flag);
				}
				return;
			}
		}
	}
}

lcf::rpg::SaveEventExecFrame::DebugFlags Debug::AnalyzeStackFrame(Game_BaseInterpreterContext const& interpreter, lcf::rpg::SaveEventExecFrame const& frame, StackFrameTraverseMode traverse_mode, const int start_index) {
	using DebugFlags = lcf::rpg::SaveEventExecFrame::DebugFlags;

	uint32_t debug_flags = 0;

	std::function<void(Game_BaseInterpreterContext const&, lcf::rpg::EventCommand const&, uint32_t&)> func = [](Game_BaseInterpreterContext const& interpreter, lcf::rpg::EventCommand const& com, uint32_t& debug_flags) {

		check_and_apply_flag_tuples(com,
			DebugFlags::DebugFlags_might_yield,
			to_span(cmds_might_yield),
			debug_flags);
		if (!interpreter.IsBackgroundInterpreter()) {
			check_and_apply_flag(com,
				DebugFlags::DebugFlags_might_yield,
				to_span(cmds_might_yield_parallel),
				debug_flags);
		}
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_branch,
			to_span(cmds_might_branch),
			debug_flags);
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_push_frame,
			to_span(cmds_might_push_frame),
			debug_flags);
		check_and_apply_flag_tuples(com,
			DebugFlags::DebugFlags_might_push_message,
			to_span(cmds_might_push_message),
			debug_flags);
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_request_scene,
			to_span(cmds_might_request_scene),
			debug_flags);
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_trigger_async_op,
			to_span(cmds_might_trigger_async_op),
			debug_flags);
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_teleport,
			to_span(cmds_might_teleport),
			debug_flags);
		check_and_apply_flag_tuples(com,
			DebugFlags::DebugFlags_might_wait,
			to_span(cmds_might_wait),
			debug_flags);
		check_and_apply_flag_tuples(com,
			DebugFlags::DebugFlags_might_refresh,
			to_span(cmds_might_refresh),
			debug_flags);
		check_and_apply_flag(com,
			DebugFlags::DebugFlags_might_gameover,
			to_span(cmds_might_gameover),
			debug_flags);
	};

	Game_Interpreter_Shared::AnalyzeStackFrame<uint32_t>(interpreter, frame, debug_flags, func, traverse_mode, start_index);

	return static_cast<DebugFlags>(debug_flags);
}


void Debug::ParallelInterpreterStates::ApplyMapChangedFlagToBackgroundInterpreters(std::vector<Game_CommonEvent>& common_events) {

	for (auto it = common_events.begin(); it < common_events.end(); ++it) {
		auto& ce = *it;
		if (ce.IsWaitingBackgroundExecution(false)) {
			auto& state = const_cast<lcf::rpg::SaveEventExecState&>(ce.interpreter->GetState());
			auto& recent_frame = state.stack[state.stack.size() - 1];
			if (state.stack.size() > 1 || recent_frame.current_command > 0) {
				recent_frame.easyrpg_runtime_flags |= lcf::rpg::SaveEventExecFrame::RuntimeFlags_map_has_changed;
			}
		}
	}
}

#endif
