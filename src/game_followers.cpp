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
#include <cassert>
#include <lcf/data.h>
#include "main_data.h"
#include "game_actors.h"
#include "game_system.h"
#include "game_map.h"
#include "game_party.h"
#include "game_player.h"
#include "game_followers.h"
#include "output.h"

namespace {
	static std::vector<Game_Follower> empty_vec;

	std::vector<Game_Follower> followers;
	bool is_following_enabled = false;
	bool global_frozen = false;
	bool global_auto_sync = true;
	bool global_awaitable = false;
}

void Game_Followers::Init() {
	followers.clear();
	followers.emplace_back(1);
	followers.emplace_back(2);
	followers.emplace_back(3);
}

std::vector<Game_Follower>& Game_Followers::GetAllFollowers() {
	if (!is_following_enabled)
		return empty_vec;
	return followers;
}

void Game_Followers::SetFollowingEnabled(bool followers_enabled) {
	is_following_enabled = followers_enabled;
}

bool Game_Followers::IsFollowingEnabled() {
	return is_following_enabled;
}

void Game_Followers::SetAllFrozen(bool frozen) {
	global_frozen = frozen;

	for (auto& follower : GetAllFollowers()) {
		follower.SetFrozen(frozen);
	}
}

void Game_Followers::SetAllAutoSync(bool auto_sync) {
	global_auto_sync = auto_sync;

	for (auto& follower : GetAllFollowers()) {
		follower.SetAutoSync(auto_sync);
	}
}

void Game_Followers::SetAllAwaitable(bool awaitable) {
	global_awaitable = awaitable;

	for (auto& follower : GetAllFollowers()) {
		follower.SetAwaitable(global_awaitable);
	}
}

void Game_Followers::ForceSyncFollowers() {
	for (auto& follower : GetAllFollowers()) {
		follower.SyncWithParty();
	}
}

void Game_Followers::ForceResetFollowers() {
	for (auto& follower : GetAllFollowers()) {
		follower.SetForceReset(true);
	}
}

Game_Follower* Game_Followers::GetFollower(int id) {
	if (id >= 1 && id <= 3) {
		return &followers[id - 1];
	}

	return nullptr;
}

Game_Follower* Game_Followers::GetFollowerByPartyPosition(int party_index) {
	return GetFollower(party_index - 1);
}

Game_Follower* Game_Followers::GetFollowerByActorId(int actor_id) {
	int party_index = Main_Data::game_party->GetActorPositionInParty(actor_id);
	return GetFollower(party_index - 1);
}

void Game_Followers::PrepareSave(lcf::rpg::SaveEasyRpgData& save) {
	save.followers_enabled = is_following_enabled;
	save.followers_frozen = global_frozen;
	save.followers_auto_sync = global_auto_sync;
	save.followers_awaitable = global_awaitable;

	save.follower1 = GetFollower(1)->GetSaveData();
	save.follower2 = GetFollower(2)->GetSaveData();
	save.follower3 = GetFollower(3)->GetSaveData();
}

void Game_Followers::SetupFromSave(lcf::rpg::SaveEasyRpgData& save) {
	is_following_enabled = save.followers_enabled;
	global_frozen = save.followers_frozen;
	global_auto_sync = save.followers_auto_sync;
	global_awaitable = save.followers_awaitable;

	GetFollower(1)->SetSaveData(std::move(save.follower1));
	GetFollower(2)->SetSaveData(std::move(save.follower2));
	GetFollower(3)->SetSaveData(std::move(save.follower3));
}


Game_Follower::Game_Follower(int party_index)
	: Game_FollowerBase(Vehicle)
{
	data()->follower_id = party_index;
	data()->is_init = true;
	SetAnimationType(AnimType::AnimType_non_continuous);
	SetLayer(lcf::rpg::EventPage::Layers_same);
	SetFrozen(global_frozen);
	SetAutoSync(global_auto_sync);
	SetAwaitable(global_awaitable);
}

void Game_Follower::SetSaveData(lcf::rpg::SaveFollowerLocation save) {
	*data() = std::move(save);
	SanitizeData("Follower" + std::to_string(data()->follower_id));
}

bool Game_Follower::IsInCurrentMap() const {
	return GetMapId() == Game_Map::GetMapId();
}

void Game_Follower::UpdateNextMovementAction() {
	if (GetActorId() > 0) {
		if (IsFrozen() && IsInCurrentMap()) {
			UpdateMoveRoute(data()->move_route_index, data()->move_route, true);
		} else if (!Main_Data::game_player->InVehicle()) {
			auto previous = GetPreviousCharacter();
			if (previous) {
				UpdateFollowMovement(*previous);
			} else {
				SetActive(false);
			}
		} else {
			SetActive(false);
		}
	} else {
		SetActive(false);
	}
}

void Game_Follower::Update() {
	if (!Game_Followers::IsFollowingEnabled) {
		SetActive(false);
		return;
	}
	if (IsAutoSync()) {
		/*if (!IsStopping() || IsStopCountActive()) {
			return;
		}*/
		SyncWithParty();
	}
	if (GetActorId() > 0 && !Main_Data::game_player->InVehicle()) {
		SetActive(true);
		SyncToPreviousCharacter();
	}
	Game_Character::Update();
}

void Game_Follower::UpdateFollowMovement(Game_Character& following) {
	if (data()->is_init) {
		SetDirection(following.GetDirection());
		SetFacing(GetDirection());
		SetThrough(true);
		data()->is_init = false;
		return;
	}

	if (!IsStopping()) {
		return;
	}

	if (GetMoveSpeed() != following.GetMoveSpeed()) {
		SetMoveSpeed(following.GetMoveSpeed());
	}

	int delta_x = GetDistanceXfromCharacter(following),
		delta_y = GetDistanceYfromCharacter(following);
	int follow_dir = following.GetDirection();

	int my_mvmt = -1;

	if (delta_x == 1 && delta_y == -1) {
		if (follow_dir == Down)
			my_mvmt = Left;
		if (follow_dir == Left)
			my_mvmt = Down;
	} else if (delta_x == 1 && delta_y == 1) {
		if (follow_dir == Up)
			my_mvmt = Left;
		if (follow_dir == Left)
			my_mvmt = Up;
	} else if (delta_x == -1 && delta_y == 1) {
		if (follow_dir == Up)
			my_mvmt = Right;
		if (follow_dir == Right)
			my_mvmt = Up;
	} else if (delta_x == -1 && delta_y == -1) {
		if (follow_dir == Down)
			my_mvmt = Right;
		if (follow_dir == Right)
			my_mvmt = Down;
	} else if (delta_x >= 2) {
		my_mvmt = Left;
		if (delta_y >= 1)
			my_mvmt = UpLeft;
		else if (delta_y <= -1)
			my_mvmt = DownLeft;
	} else if (delta_x <= -2) {
		my_mvmt = Right;
		if (delta_y >= 1)
			my_mvmt = UpRight;
		else if (delta_y <= -1)
			my_mvmt = DownRight;
	} else if (delta_y >= 2) {
		my_mvmt = Up;
		if (delta_x >= 1)
			my_mvmt = UpLeft;
		else if (delta_x <= -1)
			my_mvmt = UpRight;
	} else if (delta_y <= -2) {
		my_mvmt = Down;
		if (delta_x >= 1)
			my_mvmt = DownLeft;
		else if (delta_x <= -1)
			my_mvmt = UpLeft;
	}

	if (my_mvmt != -1) {
		Move(my_mvmt);
		SetMaxStopCountForStep();
	}
}

void Game_Follower::SyncToPreviousCharacter() {
	auto previous = GetPreviousCharacter();
	if (previous && (GetMapId() != previous->GetMapId() || IsForceReset())) {
		SetMapId(previous->GetMapId());
		SetX(previous->GetX());
		SetY(previous->GetY());
		SetForceReset(false);
		data()->is_init = true;
	}
}

void Game_Follower::SetFrozen(bool frozen) {
	data()->is_frozen = frozen;

	if (frozen) {
		SetThrough(false);
	} else {
		data()->is_init = true;
	}
}

Game_Character* Game_Follower::GetPreviousCharacter() const {
	if (GetPartyIndex() == 2)
		return Main_Data::game_player.get();
	return Game_Followers::GetFollowerByPartyPosition(GetPartyIndex() - 1);
}

void Game_Follower::SyncWithParty() {
	auto actor = Main_Data::game_party->GetActorAtPosition(data()->follower_id);
	if (actor) {
		if (GetActorId() != actor->GetId()) {
			if (GetActorId() == 0) {
				data()->is_init = true;
			}
			data()->actor_id = actor->GetId();
		}

		// sync sprite
		auto& sprite_name = actor->GetSpriteName();
		auto sprite_index = actor->GetSpriteIndex();
		if (sprite_name != this->sync_sprite_name || sprite_index != this->sync_sprite_index) {
			this->sync_sprite_name = ToString(sprite_name);
			this->sync_sprite_index = sprite_index;

			SetSpriteGraphic(ToString(sprite_name), sprite_index);
		}
	} else {
		data()->actor_id = 0;
	}
}
