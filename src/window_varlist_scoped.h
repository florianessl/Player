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

#ifndef EP_WINDOW_VARLIST_SCOPED_H
#define EP_WINDOW_VARLIST_SCOPED_H

// Headers
#include "window_selectable.h"

class Window_VarListScoped : public Window_Selectable
{
public:
	enum Mode {
		eNone,
		eMapSwitch,
		eMapVariable,
		eMapEventSwitch,
		eMapEventVariable
	};

	/**
	 * Constructor.
	 *
	 * @param commands commands to display.
	 */
	Window_VarListScoped();
	~Window_VarListScoped() override;

	/**
	 * UpdateList.
	 * 
	 * @param first_value starting value.
	 */
	void UpdateList(int first_value);

	void UpdateCursorRect() override;

	/**
	 * Refreshes the window contents.
	 */
	void  Refresh();

	/**
	 * Indicate what to display.
	 *
	 * @param mode the mode to set.
	 */
	void SetMode(Mode mode);

	/**
	 * Returns the current mode.
	 */
	Mode GetMode() const;

	void SetScope(int map_id, int evt_id);

private:

	/**
	 * Draws the value of a variable standing on a row.
	 *
	 * @param index row with the var
	 */
	void DrawItemValue(int index);

	Mode mode = eNone;
	std::vector<std::string> items;
	int first_var = 0;
	int scope_map_id = 0, scope_evt_id = 0;

	bool DataIsValid(int range_index);

};

inline Window_VarListScoped::Mode Window_VarListScoped::GetMode() const {
	return mode;
}

#endif
