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

#ifndef EP_EXE_CONSTANTS_H
#define EP_EXE_CONSTANTS_H

#include <cstdint>
#include <vector>
#include "player.h"

namespace ExeConstants {

	using Type = Player::GameConstantType;

	struct CodeAddressInfoU32 {
		int32_t default_val;
		size_t code_offset;
		std::vector<uint8_t> pre_data;
	};

	using code_address_map = std::vector<std::pair<Type, struct CodeAddressInfoU32>>;

#define ADD_EAX_ESI 0x03, 0xC6
#define MOV_EBX		0xB9
#define MOV_ECX		0xB9
#define MOV_EDX		0xBA
#define SUB_EDX_EBX 0x2B, 0xD3

#define DEPENDS_ON_PREVIOUS { 0xFF, 0xFF }

	const auto magic_prev = std::vector<uint8_t>(DEPENDS_ON_PREVIOUS);

	/*

		1.08 .CODE -> offset + 0x400
		 999 : E7 03 00 00
		9999 : 0F 27 00 00

	*/

}
namespace ExeConstants::RT_2K {

	const code_address_map const_addresses_103b = {
		{
			Type::TitleX,
			{ 160, 0x06CC39, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x06CC40, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x06CC5B, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x06CC62, { SUB_EDX_EBX, MOV_EBX } }
		}
	};

	const code_address_map const_addresses_105b = {
		{
			Type::TitleX,
			{ 160, 0x06E091, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x06E098, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x06E0B3, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x06E0BA, { SUB_EDX_EBX, MOV_EBX } }
		}
	};

	const code_address_map const_addresses_106 = {
		{
			Type::TitleX,
			{ 160, 0x06D1B9, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x06D1C0, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x06D1DB, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x06D1E2, { SUB_EDX_EBX, MOV_EBX } }
		}
	};
}

namespace ExeConstants::RT_2K3 {

	const code_address_map const_addresses_104 = {
		{
			Type::TitleX,
			{ 160, 0x08A849, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x08A850, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x08A86B, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x08A872, { SUB_EDX_EBX, MOV_EBX } }
		}
	};

	const code_address_map const_addresses_106 = {
		{
			Type::TitleX,
			{ 160, 0x08F76D, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x08F774, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x08F78F, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x08F796, { SUB_EDX_EBX, MOV_EBX } }
		}
	};

	const code_address_map const_addresses_108 = {
		{
			Type::TitleX,
			{ 160, 0x08F821, { MOV_EDX } }
		}, {
			Type::TitleY,
			{ 148, 0x08F828, { SUB_EDX_EBX, MOV_EBX } }
		}, {
			Type::TitleHiddenX,
			{ 160, 0x08F843, { MOV_EDX } }
		}, {
			Type::TitleHiddenY,
			{  88, 0x08F84A, { SUB_EDX_EBX, MOV_EBX } }
		}
	};
}


#endif
