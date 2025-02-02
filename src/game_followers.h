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

#ifndef EP_GAME_FOLLOWER_H
#define EP_GAME_FOLLOWER_H

// Headers
#include <string>
#include <vector>
#include <lcf/rpg/savefollowerlocation.h>
#include <lcf/rpg/saveeasyrpgdata.h>
#include "game_character.h"

class Game_Follower;

namespace Game_Followers {

	void Init();

	std::vector<Game_Follower>& GetAllFollowers();

	void SetFollowingEnabled(bool followers_enabled);
	bool IsFollowingEnabled();
	void SetAllFrozen(bool frozen);
	void SetAllAutoSync(bool auto_sync);
	void SetAllAwaitable(bool awaitable);
	void ForceSyncFollowers();
	void ForceResetFollowers();

	Game_Follower* GetFollower(int index);
	Game_Follower* GetFollowerByPartyPosition(int party_index);
	Game_Follower* GetFollowerByActorId(int actor_id);

	void PrepareSave(lcf::rpg::SaveEasyRpgData& save);
	void SetupFromSave(lcf::rpg::SaveEasyRpgData& save);
}

using Game_FollowerBase = Game_CharacterDataStorage<lcf::rpg::SaveFollowerLocation>;

/**
 * Game_Follower class.
 */
class Game_Follower : public Game_FollowerBase {
public:
	explicit Game_Follower(int party_index);

	/** Load from saved game */
	void SetSaveData(lcf::rpg::SaveFollowerLocation save);

	/** @return save game data */
	lcf::rpg::SaveFollowerLocation GetSaveData() const;

	void UpdateNextMovementAction() override;

	/** Update this for the current frame */
	void Update();

	bool IsInCurrentMap() const;
	bool IsInPosition(int x, int y) const override;
	bool IsVisible() const override;

	int GetPartyIndex() const;
	int GetActorId() const;
	void SetFrozen(bool frozen);
	bool IsFrozen() const;
	void SetAutoSync(bool auto_sync);
	bool IsAutoSync() const;
	void SetAwaitable(bool awaitable);
	bool IsAwaitable() const;
	void SetForceReset(bool force_reset);
	bool IsForceReset() const;

	void SyncWithParty();
private:
	Game_Character* GetPreviousCharacter() const;
	void UpdateFollowMovement(Game_Character& following);
	void SyncToPreviousCharacter();

	std::string sync_sprite_name;
	int sync_sprite_index = -1;
};

inline lcf::rpg::SaveFollowerLocation Game_Follower::GetSaveData() const {
	return *data();
}

inline int Game_Follower::GetPartyIndex() const {
	return data()->follower_id + 1;
}

inline int Game_Follower::GetActorId() const {
	return data()->actor_id;
}

inline bool Game_Follower::IsInPosition(int x, int y) const {
	return IsInCurrentMap() && Game_Character::IsInPosition(x, y);
}

inline bool Game_Follower::IsVisible() const {
	return IsInCurrentMap() && Game_Character::IsVisible();
}

inline bool Game_Follower::IsFrozen() const {
	return data()->is_frozen;
}

inline void Game_Follower::SetAutoSync(bool auto_sync) {
	data()->auto_sync = auto_sync;
}

inline bool Game_Follower::IsAutoSync() const {
	return data()->auto_sync;
}

inline void Game_Follower::SetAwaitable(bool awaitable) {
	data()->awaitable = awaitable;
}

inline bool Game_Follower::IsAwaitable() const {
	return data()->awaitable;
}

inline void Game_Follower::SetForceReset(bool force_reset) {
	data()->force_reset = force_reset;
}

inline bool Game_Follower::IsForceReset() const {
	return data()->force_reset;
}

#endif
