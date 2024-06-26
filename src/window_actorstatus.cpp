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
#include <iomanip>
#include <sstream>
#include "window_actorstatus.h"
#include "game_actors.h"
#include "game_party.h"
#include "bitmap.h"
#include "font.h"

Window_ActorStatus::Window_ActorStatus(int ix, int iy, int iwidth, int iheight, const Game_Actor& actor) :
	Window_Base(ix, iy, iwidth, iheight),
	actor(actor) {

	SetContents(Bitmap::Create(width - 16, height - 16));

	Refresh();
}

void Window_ActorStatus::Refresh() {
	contents->Clear();

	DrawStatus();
}

void Window_ActorStatus::DrawStatus() {
	int have, max;
	auto fontcolor = [&have, &max](bool can_knockout) {
		if (can_knockout && have == 0) return Font::ColorKnockout;
		if (max > 0 && (have <= max / 4)) return Font::ColorCritical;
		return Font::ColorDefault;
	};

	// Draw Hp
	contents->TextDraw(1, 2, 1, lcf::Data::terms.health_points);
	have = actor.GetHp();
	max = actor.GetMaxHp();
	DrawMinMax(90, 2, have, max, fontcolor(true));

	// Draw Sp
	contents->TextDraw(1, 18, 1, lcf::Data::terms.spirit_points);
	have = actor.GetSp();
	max = actor.GetMaxSp();
	DrawMinMax(90, 18, have, max, fontcolor(false));

	// Draw Exp
	contents->TextDraw(1, 34, 1, lcf::Data::terms.exp_short);
	DrawMinMax(90, 34, -1, -1);
}

void Window_ActorStatus::DrawMinMax(int cx, int cy, int min, int max, int color) {
	std::stringstream ss;
	if (max >= 0) {
		ss << min;
	} else {
		ss << actor.GetExpString(true);
	}
	contents->TextDraw(cx, cy, color, ss.str(), Text::AlignRight);
	contents->TextDraw(cx, cy, Font::ColorDefault, "/");
	ss.str("");
	if (max >= 0) {
		ss << max;
	} else {
		ss << actor.GetNextExpString(true);
	}
	contents->TextDraw(cx + 48, cy, Font::ColorDefault, ss.str(), Text::AlignRight);
}
