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
#include "exe_buildinfo.h"
#include "player.h"
#include "utils.h"

namespace ExeConstants {
	using namespace BuildInfo;

	enum class KnownPatchConfigurations {
		Rm2k3_Italian_WD_108,		// Italian "WhiteDragon" patch
		QP_StatDelimiter,
		LAST
	};

	static constexpr auto kKnownPatchConfigurations = lcf::makeEnumTags<KnownPatchConfigurations>(
		"Rm2k3 Italian 1.08",
		"QuickPatch StatDelimiter"
	);

	static_assert(kKnownPatchConfigurations.size() == static_cast<size_t>(KnownPatchConfigurations::LAST));

	using patch_config = std::map<Player::GameConstantType, int32_t>;
	using T = Player::GameConstantType;

	const std::map<KnownPatchConfigurations, patch_config> known_patch_configurations = {
		{
			KnownPatchConfigurations::Rm2k3_Italian_WD_108, {
				{ T::MinVarLimit,	-999999999 },
				{ T::MaxVarLimit,	 999999999 },
				{ T::MaxEnemyHP,     999999999 },
				{ T::MaxEnemySP,     999999999 },
				{ T::MaxActorHP,         99999 },
				{ T::MaxActorSP,          9999 },
				{ T::MaxAtkBaseValue,     9999 },
				{ T::MaxDefBaseValue,     9999 },
				{ T::MaxSpiBaseValue,     9999 },
				{ T::MaxAgiBaseValue,     9999 },
				{ T::MaxDamageValue,     99999 },
				{ T::MaxGoldValue,     9999999 }
		}},{
			KnownPatchConfigurations::QP_StatDelimiter, {
				{ T::MaxActorHP,       9999999 },
				{ T::MaxActorSP,       9999999 },
				{ T::MaxAtkBaseValue,   999999 },
				{ T::MaxDefBaseValue,   999999 },
				{ T::MaxSpiBaseValue,   999999 },
				{ T::MaxAgiBaseValue,   999999 },
				{ T::MaxAtkBattleValue, 999999 },
				{ T::MaxDefBattleValue, 999999 },
				{ T::MaxSpiBattleValue, 999999 },
				{ T::MaxAgiBattleValue, 999999 }
		}}
	};

	template<size_t S>
	class fixed_size_byte_array {
	public:
		template <typename... Args>
		constexpr fixed_size_byte_array(Args... args)
			: _size(sizeof...(args)), _data(init_fixedsize_array<sizeof...(args)>({ args... })) {
		}
		constexpr size_t size() const { return _size; }

		constexpr const uint8_t operator[](const size_t off) const noexcept {
			return _data[off];
		}
	private:
		template<size_t N>
		static constexpr std::array<uint8_t, S> init_fixedsize_array(const std::array<const uint8_t, N> input) {
			std::array<uint8_t, S> result {};
			for (size_t i = 0; i < N; ++i) {
				result[i] = input[i];
			}
			return result;
		}

		size_t _size;
		std::array<uint8_t, S> _data;
	};

	constexpr size_t MAX_SIZE_CHK_PRE = 4;

	using validate_const_data = fixed_size_byte_array<MAX_SIZE_CHK_PRE>;

	class CodeAddressInfo {
	public:
		int32_t default_val;
		uint8_t size_val;
		size_t code_offset;
		validate_const_data pre_data;

		template<typename... Args>
		constexpr CodeAddressInfo(int32_t default_val, uint8_t size_val, size_t code_offset, Args... args) :
			default_val(default_val), size_val(size_val), code_offset(code_offset), pre_data(validate_const_data{ static_cast<const uint8_t>(args)... }) {
		}
	};

	using code_address = std::pair<Player::GameConstantType, CodeAddressInfo>;
	using code_address_map = std::array<code_address, static_cast<size_t>(Player::GameConstantType::LAST)>;

#define ADD_EAX_ESI 0x03, 0xC6
#define ADD_EDX_ESI 0x03, 0xD6
#define MOV_EAX		0xB8
#define MOV_ECX		0xB9
#define MOV_EDX		0xBA
#define SUB_EDX_EBX 0x2B, 0xD3
#define CMP_DWORD_ESP 0x81, 0x7C, 0x24
#define CMP_ESI		0x81, 0xFE
#define CMP_EAX_BYTE 0x83, 0xF8
#define CMP_EBX_BYTE 0x83, 0xFB

//#define DEPENDS_ON_PREVIOUS { 0xFF, 0xFF, 0x00, 0xFF }
//
//	constexpr auto magic_prev = std::array<uint8_t, 4 >(DEPENDS_ON_PREVIOUS);

	template<typename T, Player::GameConstantType C, typename... Args>
	constexpr code_address map(T default_val, size_t code_offset, Args&&... args) {
		return { C, CodeAddressInfo(default_val, sizeof(T), code_offset, std::forward<decltype(args)>(args)...) };
	}

	template<Player::GameConstantType C>
	constexpr code_address not_def() {
		return { C, CodeAddressInfo(0, 0, 0) };
	}

	using engine_code_adresses_rm2k = std::array<std::pair<KnownEngineBuildVersions, code_address_map>, static_cast<size_t>(count_known_rm2k_builds)>;
	using engine_code_adresses_rm2k3 = std::array<std::pair<KnownEngineBuildVersions, code_address_map>, static_cast<size_t>(count_known_rm2k3_builds)>;

	constexpr CodeAddressInfo asdf = CodeAddressInfo(0, 0, 0, 0);

	constexpr code_address_map empty_code_map = {{
		not_def<T::MinVarLimit>(),
		not_def<T::MaxVarLimit>(),
		not_def<T::TitleX>(),
		not_def<T::TitleY>(),
		not_def<T::TitleHiddenX>(),
		not_def<T::TitleHiddenY>(),

		not_def<T::MaxActorHP>(),
		not_def<T::MaxActorSP>(),
		not_def<T::MaxEnemyHP>(),
		not_def<T::MaxEnemySP>(),

		not_def<T::MaxAtkBaseValue>(),
		not_def<T::MaxDefBaseValue>(),
		not_def<T::MaxSpiBaseValue>(),
		not_def<T::MaxAgiBaseValue>(),

		not_def<T::MaxAtkBattleValue>(),
		not_def<T::MaxDefBattleValue>(),
		not_def<T::MaxSpiBattleValue>(),
		not_def<T::MaxAgiBattleValue>(),

		not_def<T::MaxDamageValue>(),
		not_def<T::MaxExpValue>(),
		not_def<T::MaxGoldValue>(),
		not_def<T::MaxItemCount>(),
		not_def<T::MaxSaveFiles>(),
		not_def<T::MaxLevel>()
	} };

	constexpr engine_code_adresses_rm2k known_engine_builds_rm2k = {{
		{
			RM2K_20000306,
			empty_code_map
		}, {
			RM2K_2000XXXX_UNK,
			{{
				map<int32_t, T::MinVarLimit>        ( -999999, 0x085A0C, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        (  999999, 0x085A36, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x06D039, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x06D040, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x06D05B, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x06D062, SUB_EDX_EBX, MOV_ECX),

				not_def<T::MaxActorHP>(),
				not_def<T::MaxActorSP>(),
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				not_def<T::MaxAtkBaseValue>(),
				not_def<T::MaxDefBaseValue>(),
				not_def<T::MaxSpiBaseValue>(),
				not_def<T::MaxAgiBaseValue>(),

				not_def<T::MaxAtkBattleValue>(),
				not_def<T::MaxDefBattleValue>(),
				not_def<T::MaxSpiBattleValue>(),
				not_def<T::MaxAgiBattleValue>(),

				not_def<T::MaxDamageValue>(),
				not_def<T::MaxExpValue>(),
				not_def<T::MaxGoldValue>(),
				not_def<T::MaxItemCount>(),
				not_def<T::MaxSaveFiles>(),
				not_def<T::MaxLevel>()
			}}
		}, {
			RM2K_20000507,
			empty_code_map
		},{
			RM2K_20000619,
			empty_code_map
		},{
			RM2K_20000711,
			{{
				map<int32_t, T::MinVarLimit>        ( -999999, 0x0846A8, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        (  999999, 0x0846D2, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x06E491, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x06E498, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x06E4B3, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x06E4BA, SUB_EDX_EBX, MOV_ECX),

				not_def<T::MaxActorHP>(),
				not_def<T::MaxActorSP>(),
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				not_def<T::MaxAtkBaseValue>(),
				not_def<T::MaxDefBaseValue>(),
				not_def<T::MaxSpiBaseValue>(),
				not_def<T::MaxAgiBaseValue>(),

				not_def<T::MaxAtkBattleValue>(),
				not_def<T::MaxDefBattleValue>(),
				not_def<T::MaxSpiBattleValue>(),
				not_def<T::MaxAgiBattleValue>(),

				not_def<T::MaxDamageValue>(),
				not_def<T::MaxExpValue>(),
				not_def<T::MaxGoldValue>(),
				not_def<T::MaxItemCount>(),
				not_def<T::MaxSaveFiles>(),
				not_def<T::MaxLevel>()
			}}
		},{
			RM2K_20001113,
			empty_code_map
		},{
			RM2K_20001115,
			empty_code_map
		},{
			RM2K_20001227,
			{{
				map<int32_t, T::MinVarLimit>        ( -999999, 0x085D78, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        (  999999, 0x085DA2, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x06D5B9, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x06D5C0, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x06D5DB, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x06D5E2, SUB_EDX_EBX, MOV_ECX),

				not_def<T::MaxActorHP>(),
				not_def<T::MaxActorSP>(),
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				not_def<T::MaxAtkBaseValue>(),
				not_def<T::MaxDefBaseValue>(),
				not_def<T::MaxSpiBaseValue>(),
				not_def<T::MaxAgiBaseValue>(),

				not_def<T::MaxAtkBattleValue>(),
				not_def<T::MaxDefBattleValue>(),
				not_def<T::MaxSpiBattleValue>(),
				not_def<T::MaxAgiBattleValue>(),

				not_def<T::MaxDamageValue>(),
				not_def<T::MaxExpValue>(),
				not_def<T::MaxGoldValue>(),
				not_def<T::MaxItemCount>(),
				not_def<T::MaxSaveFiles>(),
				not_def<T::MaxLevel>()
			}}
		},{
			RM2K_20010505,
			empty_code_map
		},{
			RM2K_20030327,
			empty_code_map
		},{
			RM2K_20030625,
			empty_code_map
		},{
			RM2KE_160,
			empty_code_map
		},{
			RM2KE_161,
			empty_code_map
		},{
			RM2KE_162,
			empty_code_map
		}
	}};

	constexpr engine_code_adresses_rm2k3 known_engine_builds_rm2k3 = {{
		{
			RM2K3_100,
			empty_code_map
		}, {
			RM2K3_UNK_1,
			empty_code_map
		},{
			RM2K3_UNK_2,
			empty_code_map
		}, {
			RM2K3_1021_1021,
			empty_code_map
		},{
			RM2K3_1030_1030_1,
			empty_code_map
		}, {
			RM2K3_1030_1030_2,
			empty_code_map
		},{
			RM2K3_1030_1040,
			{{
				map<int32_t, T::MinVarLimit>        (-9999999, 0x0A60B3, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        ( 9999999, 0x0A60DD, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x08AC49, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x08AC50, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x08AC6B, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x08AC72, SUB_EDX_EBX, MOV_ECX),

				not_def<T::MaxActorHP>(),
				not_def<T::MaxActorSP>(),
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				not_def<T::MaxAtkBaseValue>(),
				not_def<T::MaxDefBaseValue>(),
				not_def<T::MaxSpiBaseValue>(),
				not_def<T::MaxAgiBaseValue>(),

				not_def<T::MaxAtkBattleValue>(),
				not_def<T::MaxDefBattleValue>(),
				not_def<T::MaxSpiBattleValue>(),
				not_def<T::MaxAgiBattleValue>(),

				not_def<T::MaxDamageValue>(),
				not_def<T::MaxExpValue>(),
				not_def<T::MaxGoldValue>(),
				not_def<T::MaxItemCount>(),
				not_def<T::MaxSaveFiles>(),
				not_def<T::MaxLevel>()
			}}
		}, {
			RM2K3_1050_1050_1,
			empty_code_map
		},{
			RM2K3_1050_1050_2,
			empty_code_map
		}, {
			RM2K3_1060_1060,
			{{
				map<int32_t, T::MinVarLimit>        (-9999999, 0x0AC4F7, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        ( 9999999, 0x0AC521, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x08FB6D, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x08FB74, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x08FB8F, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x08FB96, SUB_EDX_EBX, MOV_ECX),

				not_def<T::MaxActorHP>(),
				not_def<T::MaxActorSP>(),
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				not_def<T::MaxAtkBaseValue>(),
				not_def<T::MaxDefBaseValue>(),
				not_def<T::MaxSpiBaseValue>(),
				not_def<T::MaxAgiBaseValue>(),

				not_def<T::MaxAtkBattleValue>(),
				not_def<T::MaxDefBattleValue>(),
				not_def<T::MaxSpiBattleValue>(),
				not_def<T::MaxAgiBattleValue>(),

				not_def<T::MaxDamageValue>(),
				not_def<T::MaxExpValue>(),
				not_def<T::MaxGoldValue>(),
				not_def<T::MaxItemCount>(),
				not_def<T::MaxSaveFiles>(),
				not_def<T::MaxLevel>()
			}}
		},{
			RM2K3_1070_1070,
			empty_code_map
		},{
			RM2K3_1080_1080,
			{{
				map<int32_t, T::MinVarLimit>        (-9999999, 0x0AC76B, CMP_DWORD_ESP, 0x10),
				map<int32_t, T::MaxVarLimit>        ( 9999999, 0x0AC795, CMP_DWORD_ESP, 0x10),

				map<int32_t, T::TitleX>             (     160, 0x08FC21, MOV_EDX),
				map<int32_t, T::TitleY>             (     148, 0x08FC28, SUB_EDX_EBX, MOV_ECX),
				map<int32_t, T::TitleHiddenX>       (     160, 0x08FC43, MOV_EDX),
				map<int32_t, T::TitleHiddenY>       (      88, 0x08FC4A, SUB_EDX_EBX, MOV_ECX),

				map<int32_t, T::MaxActorHP>         (    9999, 0x0B652B, MOV_ECX), /* 0x0B858B */
				map<int32_t, T::MaxActorSP>         (     999, 0x0B659D, MOV_ECX), /* 0x0B85AD */
				not_def<T::MaxEnemyHP>(),
				not_def<T::MaxEnemySP>(),

				map<int32_t, T::MaxAtkBaseValue>    (     999, 0x0B6636, MOV_ECX), /* 0xB85CC */
				map<int32_t, T::MaxDefBaseValue>    (     999, 0x0B689C, MOV_ECX), /* 0xB85EB */
				map<int32_t, T::MaxSpiBaseValue>    (     999, 0x0B694C, MOV_ECX), /* 0xB860A */
				map<int32_t, T::MaxAgiBaseValue>    (     999, 0x0B69F2, MOV_ECX), /* 0xB8629 */
		
				map<int32_t, T::MaxAtkBattleValue>  (    9999, 0x0BEF3C, MOV_ECX),
				map<int32_t, T::MaxDefBattleValue>  (    9999, 0x0BF008, MOV_ECX),
				map<int32_t, T::MaxSpiBattleValue>  (    9999, 0x0BF0D1, MOV_ECX),
				map<int32_t, T::MaxAgiBattleValue>  (    9999, 0x0BF16D, MOV_ECX),

				map<int32_t, T::MaxDamageValue>     (    9999, 0x09C43C, MOV_EAX),
				map<int32_t, T::MaxExpValue>        ( 9999999, 0x0B60C3, CMP_ESI), /* 0xB8482 */
				map<int32_t, T::MaxGoldValue>       (  999999, 0x0A5B54, ADD_EDX_ESI, MOV_EAX),
				map<uint8_t, T::MaxItemCount>       (      99, 0x092399, CMP_EAX_BYTE),
				map<uint8_t, T::MaxSaveFiles>       (      16, 0x08FB34, CMP_EBX_BYTE),
				not_def<T::MaxLevel>(),
			}}
		},{
			RM2K3_1091_1091,
			empty_code_map
		}
	}};

	enum class KnownPatches {
		UnlockPics,
		DirectMenu,

		LAST
	};

	static constexpr auto kKnownPatches = lcf::makeEnumTags<KnownPatches>(
		"UnlockPics",
		"DirectMenu"
	);

	static_assert(kKnownPatches.size() == static_cast<size_t>(KnownPatches::LAST));

	constexpr size_t MAX_SIZE_CHK_PATCH_SEGMENT = 8;

	using patch_segment_data = fixed_size_byte_array<MAX_SIZE_CHK_PATCH_SEGMENT>;

	class PatchDetectionInfo {
	public:
		size_t chk_segment_offset;
		patch_segment_data chk_segment_data;
		size_t extract_var_offset;

		constexpr PatchDetectionInfo(size_t chk_segment_offset, patch_segment_data chk_segment_data) :
			chk_segment_offset(chk_segment_offset), chk_segment_data(chk_segment_data), extract_var_offset(0) {
		}
		constexpr PatchDetectionInfo(size_t chk_segment_offset, patch_segment_data chk_segment_data, size_t extract_var_offset) :
			chk_segment_offset(chk_segment_offset), chk_segment_data(chk_segment_data), extract_var_offset(extract_var_offset) {
		}
	};

	template<typename... Args>
	constexpr patch_segment_data patch_segment(Args... args) {
		return patch_segment_data{ static_cast<const uint8_t>(args)... };
	}

	using patch_detection = std::pair<KnownPatches, PatchDetectionInfo>;
	template<size_t S>
	using patch_detection_map = std::array<patch_detection, S>;
	
	constexpr patch_detection_map<2> patches_RM2K3_1080 = {{
		{ KnownPatches::UnlockPics,        PatchDetectionInfo(0x0B12FA, patch_segment(0x90, 0x90, 0x90, 0x90, 0x90) )},
		{ KnownPatches::DirectMenu,        PatchDetectionInfo(0x0A0422, patch_segment(0xE9, 0xE2, 0x5E, 0xFA, 0xFF), 0x00462DE)}
	}};

	constexpr patch_detection_map<1> patches_RM2K3_1091 = {{
		{ KnownPatches::DirectMenu,        PatchDetectionInfo(0x09F756, patch_segment(0xE9, 0xAE, 0x6B, 0xFA, 0xFF), 0x00462DE)}
	}};

}

#endif
