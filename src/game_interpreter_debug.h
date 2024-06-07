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


#ifndef EP_GAME_INTERPRETER_DEBUG
#define EP_GAME_INTERPRETER_DEBUG

#include "game_interpreter_shared.h"
#include "game_character.h"
#include "player.h"

class Game_CommonEvent;

namespace Debug {
	enum EasyRpgTrigger : uint8_t {
		eTrigger_action = 0,
		eTrigger_touched,
		eTrigger_collision,
		eTrigger_auto_start,
		eTrigger_parallel,
		eTrigger_called,
		eTrigger_battle_start,
		eTrigger_battle_parallel,
		eTrigger_map_init_deferred,
		eTrigger_map_init_immediate,
		eTrigger_bitmask_type = 63,
		eTrigger_flag_indirect_map_call = 64
	};

	enum StackFramePushType {
		ePush_Initial = 0,
		ePush_CallEvent,
		ePush_DeathHandler,
		ePush_Eval,
		ePush_Debug
	};

	class ParallelInterpreterStates {
	private:
		std::vector<int> ev_ids;
		std::vector<int> ce_ids;

		std::vector<lcf::rpg::SaveEventExecState> state_ev;
		std::vector<lcf::rpg::SaveEventExecState> state_ce;

		ParallelInterpreterStates(std::vector<int> ev_ids, std::vector<int> ce_ids,
			std::vector<lcf::rpg::SaveEventExecState> state_ev, std::vector<lcf::rpg::SaveEventExecState> state_ce)
		: ev_ids(ev_ids), ce_ids(ce_ids), state_ev(state_ev), state_ce(state_ce) { }
	public:
		ParallelInterpreterStates() = default;

		inline int CountEventInterpreters() const { return ev_ids.size(); }
		inline int CountCommonEventInterpreters() const { return ce_ids.size(); }

		inline int Count() { return ev_ids.size() + ce_ids.size(); }

		inline std::tuple<const int&, const lcf::rpg::SaveEventExecState&> GetEventInterpreter(int i) const {
			return std::tie(ev_ids[i], state_ev[i]);
		}
		inline std::tuple<const int&, const lcf::rpg::SaveEventExecState&> GetCommonEventInterpreter(int i) const {
			return std::tie(ce_ids[i], state_ce[i]);
		}

		static ParallelInterpreterStates GetCachedStates();
	
#ifdef INTERPRETER_DEBUGGING
		static void ApplyMapChangedFlagToBackgroundInterpreters(std::vector<Game_CommonEvent> &common_events);
#endif
	};

	struct CallStackItem {
		bool is_ce;
		int evt_id, page_id;
		std::string name;
		int stack_item_no, cmd_current, cmd_count;
	};

	std::vector<CallStackItem> CreateCallStack(const int owner_evt_id, const lcf::rpg::SaveEventExecState& state);

	std::string FormatEventName(Game_Character const& ch);
	std::string FormatEventName(Game_CommonEvent const& ev);
	std::string FormatEventName(lcf::rpg::SaveEventExecFrame const* frame);

	void AssertBlockedMoves();
}

#ifdef INTERPRETER_DEBUGGING

class Game_DebuggableInterpreterContext {
public:
	virtual bool CanHaltExecution() = 0;
	virtual bool IsHalted() = 0;
	virtual void HaltExecution() = 0;
	virtual void ResumeExecution(bool skipAssertsForCurrentCommand) = 0;
};

namespace Debug {
	using namespace Game_Interpreter_Shared;
	using Cmd = lcf::rpg::EventCommand::Code;

	extern Game_DebuggableInterpreterContext* active_interpreter;
	extern bool is_main_halted;
	extern bool in_execute_command;

	//TODO: special commands through Comments!

	static std::array cmds_might_yield = {
		//std::tuple { Cmd::Wait,				-1 }, 	//wait_enter flag only TODO!!
		std::tuple { Cmd::ShowChoice,		-1 },
		std::tuple { Cmd::EndLoop,			 0 },	// (Maniac Loop)
		std::tuple { Cmd::OpenVideoOptions, -1 },
		//Map
		std::tuple { Cmd::EnemyEncounter,	-1 },
		std::tuple { Cmd::OpenShop,			-1 },
		std::tuple { Cmd::EnterHeroName,	-1 },
		std::tuple { Cmd::OpenSaveMenu,		-1 },
		std::tuple { Cmd::OpenMainMenu,		-1 },
		std::tuple { Cmd::OpenLoadMenu,		-1 },
		//Battle
		std::tuple { Cmd::TerminateBattle,  -1 }
	};
	static std::array cmds_might_yield_parallel = {
		//Map
		Cmd::Teleport,
		Cmd::RecallToLocation,
		Cmd::EraseEvent,
	};
	static std::array cmds_might_branch = {
		Cmd::JumpToLabel,
		Cmd::ConditionalBranch,
		Cmd::Loop,
		Cmd::BreakLoop,
		Cmd::ShowChoice,
		//Map
		Cmd::EnemyEncounter,
		Cmd::OpenShop,
		Cmd::ShowInn,
		//Battle
		Cmd::ConditionalBranch_B
	};
	static std::array cmds_might_push_frame = {
		Cmd::CallEvent,
		Cmd::CallCommonEvent,
		Cmd::Maniac_CallCommand,
		static_cast <Cmd>(2002)				// Cmd::EasyRpg_TriggerEventAt
	};
	static std::array cmds_might_push_message = {
		std::tuple { Cmd::ShowMessage,		-1 },
		std::tuple { Cmd::ShowMessage_2,	-1 },
		std::tuple { Cmd::ShowChoice,		-1 },
		std::tuple { Cmd::InputNumber,		-1 },
		std::tuple { Cmd::ChangeExp,		 5 },	// ForegroundTextPush
		std::tuple { Cmd::ChangeLevel,		 5 },	// ForegroundTextPush
		std::tuple { Cmd::ChangeClass,		 6 },	// ForegroundTextPush
		//Map
		std::tuple { Cmd::ShowInn,			-1 }
	};
	static std::array cmds_might_request_scene = {	// (GameOver scene is not considered here)
		//Map
		Cmd::EnemyEncounter,
		Cmd::OpenShop,
		Cmd::EnterHeroName,
		Cmd::OpenSaveMenu,
		Cmd::OpenLoadMenu,
		Cmd::OpenMainMenu,
		Cmd::OpenVideoOptions
	};
	static std::array cmds_might_trigger_async_op = {
		Cmd::EraseScreen,
		Cmd::ShowScreen,
		Cmd::ShowPicture,
		Cmd::MovePicture,
		Cmd::ChangePBG,
		Cmd::ReturntoTitleScreen,
		Cmd::ExitGame,
		Cmd::Maniac_Save,
		Cmd::Maniac_Load,
		Cmd::Maniac_ShowStringPicture,
		Cmd::Maniac_GetPictureInfo,
		Cmd::Maniac_ControlStrings,		// File Load only
		Cmd::SetVehicleLocation,		// ForegroundInterpreter only, special case 'QuickTeleport
		//Map
		Cmd::ShowInn,
		//Battle
		Cmd::TerminateBattle
	};
	static std::array cmds_might_teleport = {
		//Map
		Cmd::Teleport,
		Cmd::RecallToLocation,
		Cmd::SetVehicleLocation,		 // ForegroundInterpreter only, special case 'QuickTeleport
		Cmd::EnemyEncounter				 // RPG2K3 DeathHandler only
	};
	static std::array cmds_might_wait = {
		std::tuple { Cmd::Wait,						-1 },
		std::tuple { Cmd::ProceedWithMovement,		-1 },
		std::tuple { Cmd::TintScreen,				 5 },
		std::tuple { Cmd::FlashScreen,				 5 },
		std::tuple { Cmd::ShakeScreen,				 3 },
		std::tuple { Cmd::MovePicture,				15 },
		std::tuple { Cmd::KeyInputProc,				 1 },
		//Map
		std::tuple { Cmd::PanScreen,				4 },
		std::tuple { Cmd::FlashSprite,				6 },
		std::tuple { Cmd::ShowBattleAnimation,		2 },
		//Battle
		std::tuple { Cmd::ShowBattleAnimation_B,	2 }
	};
	static std::array cmds_might_refresh = {
		std::tuple { Cmd::ControlSwitches,			-1 },
		std::tuple { Cmd::ControlVars,				-1 },
		std::tuple { Cmd::ChangeItems,				-1 },
		std::tuple { Cmd::ChangePartyMembers,		-1 },
		std::tuple { Cmd::SimulatedAttack,			 6 },
		std::tuple { Cmd::MemorizeLocation,			-1 },
		std::tuple { Cmd::StoreTerrainID,			-1 },
		std::tuple { Cmd::StoreEventID,				-1 },
		std::tuple { Cmd::KeyInputProc,				-1 },
		std::tuple { Cmd::InputNumber,				-1 },
		std::tuple { Cmd::TimerOperation,			-1 },
		std::tuple { Cmd::MoveEvent,				-1 },	// only special move types
		std::tuple { Cmd::Loop,						 4 },	// (Maniac Loop)
		std::tuple { Cmd::EndLoop,					 4 },	// (Maniac Loop)

		std::tuple { Cmd::Maniac_ControlStrings,	-1 },	// only special string options
		std::tuple { Cmd::Maniac_GetSaveInfo,		-1 },
		std::tuple { Cmd::Maniac_GetMousePosition,	-1 },
		std::tuple { Cmd::Maniac_GetPictureInfo,	-1 },
		std::tuple { Cmd::Maniac_ControlVarArray,	-1 },
		std::tuple { Cmd::Maniac_KeyInputProcEx,	-1 },
		std::tuple { Cmd::Maniac_ControlGlobalSave, -1 },
		std::tuple { Cmd::Maniac_AddMoveRoute,		-1 },	// only special move types

		std::tuple { Cmd::ControlSwitchesEx,		-1 },
		std::tuple { Cmd::ControlVarsEx,			-1 },
		std::tuple { Cmd::ControlScopedSwitches,	-1 },
		std::tuple { Cmd::ControlScopedVars,		-1 }
	};
	static std::array cmds_might_gameover = {
		Cmd::GameOver,
		Cmd::ChangePartyMembers,
		Cmd::ChangeExp,
		Cmd::ChangeLevel,
		Cmd::ChangeParameters,
		Cmd::ChangeSkills,
		Cmd::ChangeEquipment,
		Cmd::ChangeHP,
		Cmd::ChangeSP,
		Cmd::ChangeCondition,
		Cmd::FullHeal,
		Cmd::SimulatedAttack,
		Cmd::ChangeClass,
		// Map
		Cmd::EnemyEncounter				//Only if com.parameters[4] == 0
	};

	void LogCallback(LogLevel lvl, std::string const& msg, LogCallbackUserData userdata);

	lcf::rpg::SaveEventExecFrame::DebugFlags AnalyzeStackFrame(Game_BaseInterpreterContext const& interpreter, lcf::rpg::SaveEventExecFrame const& frame, StackFrameTraverseMode traverse_mode = eStackFrameTraversal_All, const int start_index = 0);
}

#endif

#endif
