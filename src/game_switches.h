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

#ifndef EP_GAME_SWITCHES_H
#define EP_GAME_SWITCHES_H

// Headers
#include <vector>
#include <string>
#include <string_view.h>
#include "game_scoped_storage.h"
#include <lcf/rpg/database.h>
#include "compiler.h"
#include "output.h"

/**
 * Game_Switches class
 */
class Game_Switches : public Game_SwitchesBase {
public:
	Game_Switches();

	SCOPED_WITH_DEFAULT game_bool Flip(int id, Args... args);
	SCOPED_WITH_DEFAULT void FlipRange(int first_id, int last_id, Args... args);

	StringView GetName(int id, DataScopeType scope = DataScopeType::eDataScope_Global) const override;

	SCOPED_WITH_DEFAULT int GetInt(int switch_id, Args... args) const;

protected:
	void AssignOpImpl(game_bool& target, game_bool value) const;

	uint8_t scopedInitFlags(DataScopeType scope, int id) const override;
	game_bool scopedGetDefaultValue(DataScopeType scope, int id) const override;
	void scopedGetDataFromSaveElement(lcf::rpg::SaveScopedSwitchData element, DataScopeType& scope, int& id, game_bool& value, int& map_id, int& event_id, bool& reset_flag) const override;
	lcf::rpg::SaveScopedSwitchData scopedCreateSaveElement(DataScopeType scope, int id, game_bool value, int map_id, int event_id, bool reset_flag) const override;
};

inline void Game_SwitchesBase::AssignOpImpl(game_bool& target, game_bool value) const {
	target = value;
}

SCOPED_IMPL inline int Game_Switches::GetInt(int switch_id, Args... args) const {
	return Get<S>(switch_id, args...) ? 1 : 0;
}

#endif

