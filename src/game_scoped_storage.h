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

#ifndef EP_GAME_SCOPED_STORAGE_H
#define EP_GAME_SCOPED_STORAGE_H

// Headers
#include <vector>
#include <string>
#include <string_view.h>
#include <lcf/rpg/database.h>
#include "lcf/data.h"
#ifndef SCOPEDVARS_LIBLCF_STUB
#include <lcf/rpg/savescopedswitchdata.h>
#include <lcf/rpg/savescopedvariabledata.h>
#endif
#include <lcf/rpg/saveeventexecframe.h>
#include "compiler.h"
#include "output.h"

enum DataScopeType {
	eDataScope_Global = 0,
	eDataScope_Frame = 1,
	eDataScope_Frame_CarryOnPush = 2,
	eDataScope_Frame_CarryOnPop = 3,
	eDataScope_Frame_CarryOnBoth = 4,
	eDataScope_Map = 5,
	eDataScope_MapEvent = 6,

	eDataScope_MAX = eDataScope_MapEvent
};

// Use a whole byte for switches instead of a bitmask?
// (std::vector implementation differs for boolean data types)
//typedef unsigned char game_bool;
typedef bool game_bool;

namespace VarStorage {

	enum DataStorageType : std::int8_t {
		eStorageType_Switch = 0,
		eStorageType_Variable = 1,
		eStorageType_String = 2,
		eStorageType_Json = 3
	};

	enum DataStorageMode {
		eStorageMode_Vector = 0,
		eStorageMode_Map,
		eStorageMode_Mixed
	};

	constexpr StringView TypeToStr(int type) {
		switch (type) {
			case eStorageType_Switch: return "Sw";
			case eStorageType_Variable: return "Var";
			case eStorageType_String: return "VarStr";
			case eStorageType_Json: return "Json";
		}
		return "Unk";
	}

	template <DataStorageMode storage_mode>
	constexpr bool IsVector = (storage_mode == eStorageMode_Vector);

	template <DataStorageMode storage_mode>
	constexpr bool IsMap = (storage_mode == eStorageMode_Map);

	template <DataStorageMode storage_mode>
	constexpr bool IsMixed = (storage_mode == eStorageMode_Mixed);


	template <typename V>
	struct vector_map_mixed {
		size_t bounds_vector = 200;
		int max_key_map = 0;
		std::vector<V> vec;
		std::unordered_map<size_t, V> map;

		size_t size() noexcept {
			if (map.empty())
				return vec.size();
			return static_cast<size_t>(max_key_map);
		}
	};

	template <typename V, DataStorageMode storage_mode>
	struct DataStorage_t {
		using storage_t = std::conditional_t<storage_mode == eStorageMode_Vector,
			std::vector<V>,
			std::conditional_t<storage_mode == eStorageMode_Map,
				std::unordered_map<int, V>,
				vector_map_mixed<V>
			>
		>;

		using reference = std::conditional_t<std::is_same<V, bool>::value, std::vector<bool>::reference, V&>;
		using const_reference = std::conditional_t<std::is_same<V, bool>::value, std::vector<bool>::const_reference, V const&>;

		template <DataStorageMode S = storage_mode, typename std::enable_if<IsMixed<S>, int>::type = 0>
		inline void setVectorBounds(size_t bounds) noexcept {
			this->data.bounds_vector = bounds;
		}

		inline const bool containsKey(const int id) const {
			if constexpr (IsVector<storage_mode>) {
				return id > 0 && id <= static_cast<int>(data.size());
			} else if constexpr (IsMap<storage_mode>) {
				auto it = data.find(id);
				return it != data.end();
			} else {
				return (id <= data.bounds_vector)
					? (id > 0 && id <= static_cast<int>(data.vec.size()))
					: (data.map.find(id) != data.map.end());
			}
		};

		inline reference operator[](const int id) noexcept {
			return data[id - 1];
		};
		inline const const_reference operator[](const int id) const noexcept {
			return data[id - 1];
		};

		template <DataStorageMode S = storage_mode, typename std::enable_if<IsMap<S>, int>::type = 0>
		inline reference operator[](const int id) noexcept {
			auto it = data.find(id);
			return it->second;
		};

		template <DataStorageMode S = storage_mode, typename std::enable_if<IsMap<S>, int>::type = 0>
		inline const reference operator[](const int id) const noexcept {
			auto it = data.find(id);
			return it->second;
		};

		inline void prepare(const int first_id, const int last_id) {
			if constexpr (IsVector<storage_mode>) {
				if (EP_UNLIKELY(last_id > static_cast<int>(data.size()))) {
					data.resize(last_id, false);
				}
			} else if constexpr (IsMixed<storage_mode>) {
				if (EP_UNLIKELY(last_id <= data.bounds_vector && last_id > static_cast<int>(data.vec.size()))) {
					data.vec.resize(last_id, false);
				} else if (EP_UNLIKELY(last_id > data.max_key_map)) {
					data.max_key_map = last_id;
				}
			}
		};

		const size_t size() const noexcept {
			return data.size();
		}

		template <typename V2 = V, typename std::enable_if<std::is_same<V2, bool>::value, int>::type = 0>
		inline void flip(const int id) {
			this->data.flip(id - 1);
		}

		inline void setData(std::vector<V> data) {
			if constexpr (IsVector<storage_mode>) {
				this->data = std::move(data);
			} else if constexpr (IsMap<storage_mode>) {
				//TODO
			} else {
				//TODO
			}
		}
		inline const std::vector<V> getData() const {
			if constexpr (IsVector<storage_mode>) {
				return this->data;
			} else if constexpr (IsMap<storage_mode>) {
				//TODO
			} else {
				//TODO
			}
		}

		inline void prepare_iterate(const int first_id, const int last_id) {
			static_assert(!IsMap<storage_mode>);

			if constexpr (IsVector<storage_mode>) {
				return prepare(first_id, last_id);
			} else if constexpr (IsMixed<storage_mode>) {
				if (EP_UNLIKELY(last_id <= data.bounds_vector && last_id > static_cast<int>(data.vec.size()))) {
					data.vec.resize(last_id, false);
				} else if (EP_UNLIKELY(last_id > data.max_key_map)) {
					data.max_key_map = last_id + 1;
					data.bounds_vector = last_id;
					data.vec.resize(last_id, false);

					for (auto& it = data.map.begin(); it != data.map.end();) {
						if (it.first < data.max_key_map) {
							it = data.map.erase(it);
						} else {
							it++;
						}
					}
				}
			}
		}

		inline typename std::vector<V>::iterator begin() noexcept {
			static_assert(!IsMap<storage_mode>);
			return data.begin();
		}

		inline typename std::vector<V>::const_iterator begin() const noexcept {
			static_assert(!IsMap<storage_mode>);
			return data.begin();
		}

		inline typename std::vector<V>::iterator end() noexcept {
			static_assert(!IsMap<storage_mode>);
			return data.end();
		}

		inline typename std::vector<V>::const_iterator end() const noexcept {
			static_assert(!IsMap<storage_mode>);
			return data.end();
		}

	private:
		storage_t data;
	};
}

namespace DynamicScope {

	constexpr size_t scopedvar_maps_default_count = 12;
	constexpr size_t scopedvar_mapevents_default_count = 4;

	constexpr size_t scopedvar_frame_max_count = 255;  // soft-capped to 255
	constexpr size_t scopedvar_maps_max_count = 255;  // soft-capped to 255
	constexpr size_t scopedvar_mapevents_max_count = 8; // hard-capped to 8 due to how parameters are packed in interpreter commands

	constexpr int scopedvar_max_map_id = 9999;
	constexpr int scopedvar_max_event_id = (0x7FFFFFF / 10000) - 1;
	constexpr int scopedvar_max_var_id_for_uint32_packing = 9999;
	constexpr int namedvar_max_varname_length = 32;

	constexpr int count_global_scopes = 1;

	constexpr bool IsGlobalScope(DataScopeType scope) {
		return scope == DataScopeType::eDataScope_Global;
	}

	constexpr bool IsFrameScope(DataScopeType scope) {
		return scope >= DataScopeType::eDataScope_Frame && scope <= DataScopeType::eDataScope_Frame_CarryOnBoth;
	}

	constexpr bool IsMapScope(DataScopeType scope) {
		return scope == DataScopeType::eDataScope_Map;
	}

	constexpr bool IsMapEventScope(DataScopeType scope) {
		return scope == DataScopeType::eDataScope_MapEvent;
	}

	constexpr bool IsVariableScope(DataScopeType scope) {
		return IsFrameScope(scope) || IsMapScope(scope) || IsMapEventScope(scope);
	}

	constexpr StringView ScopeToStr(DataScopeType scope) {
		switch (scope) {
		case DataScopeType::eDataScope_Global: return "";
		case DataScopeType::eDataScope_Frame:
		case DataScopeType::eDataScope_Frame_CarryOnPush:
		case DataScopeType::eDataScope_Frame_CarryOnPop:
		case DataScopeType::eDataScope_Frame_CarryOnBoth:
			return "Frame";
		case DataScopeType::eDataScope_Map: return "Map";
		case DataScopeType::eDataScope_MapEvent: return "Self";
		}
		return "Unk";
	}

	template <typename Data_t, typename V>
	class ScopedDataStorage {
	public:
		enum Flags : uint8_t {
			eFlag_ReadOnly = 0x01,
			eFlag_AutoReset = 0x02,
			eFlag_ValueDefined = 0x04,
			eFlag_DefaultValueDefined = 0x08,
			eFlag_MapGrpInheritedValue = 0x10
		};

		inline const bool containsKey(const int id) const {
			return data.size();
		};
		inline typename Data_t::reference operator[](const int id) noexcept {
			return data[id];
		};
		inline typename Data_t::const_reference operator[](const int id) const noexcept {
			return data[id];
		};
		inline void prepare(const int first_id, const int last_id) {
			data.prepare(first_id, last_id);
		};
		const size_t size() const noexcept {
			return data.size();
		}
		inline void setData(std::vector<V> data) {
			return this->data.setData(data);
		}
		inline std::vector<V> getData() const {
			return this->data.getData();
		}
		bool valid = false;
		int map_id = 0, evt_id = 0;
		std::unordered_map<int, int> flags;
	private:
		Data_t data;
	};
}

#define SCOPED_WITH_DEFAULT template<DataScopeType S = DataScopeType::eDataScope_Global, typename... Args>
#define SCOPED_IMPL template<DataScopeType S, typename... Args>
#define SCOPED_GLOBAL_ONLY template<DataScopeType S = DataScopeType::eDataScope_Global> //TODO: requires DynamicScope::IsGlobalScope(DataScopeType)
#define SCOPED_DYN_ONLY template<DataScopeType S> //TODO: requires DynamicScope::IsVariableScope(DataScopeType)
#define SCOPED_MAP_ONLY template<DataScopeType S> //TODO: requires DynamicScope::IsMapScope(DataScopeType)

#define ASSERT_IS_GLOBAL_SCOPE(S, ERR) static_assert(DynamicScope::IsGlobalScope(S), "Invalid scope for " ERR "!");
#define ASSERT_IS_VARIABLE_SCOPE(S, ERR) static_assert(DynamicScope::IsVariableScope(S), "Invalid scope for " ERR "!");

#define ASSERT_IS_MAP_SCOPE(S, ERR) static_assert(DynamicScope::IsMapScope(S) , "Invalid scope for " ERR "!");
#define ASSERT_IS_MAPEVENT_SCOPE(S, ERR) static_assert(DynamicScope::IsMapEventScope(S), "Invalid scope for " ERR "!");

namespace DynamicScope {
	using limit_array_t = std::array <size_t, (eDataScope_MAX + 1)>;
	template<typename Data_t> using globals_array_t = std::array<Data_t, DynamicScope::count_global_scopes>;

	template<DataScopeType, typename Data_t> constexpr bool IsValid(limit_array_t const& limits, globals_array_t<Data_t> const& globals, const int id);
	template<DataScopeType> constexpr size_t GetLimit(limit_array_t const& limits);
	template<DataScopeType> constexpr void SetLowerLimit(limit_array_t& limits, size_t limit);
	template<DataScopeType, typename Data_t> constexpr int GetSize(globals_array_t<Data_t> const& globals);
	template<DataScopeType, typename Data_t> constexpr int GetSizeWithLimit(limit_array_t const& limits, globals_array_t<Data_t> const& globals);

	constexpr void InitLimits(limit_array_t& limits) {
		limits[eDataScope_Global] = 0;
		limits[eDataScope_Frame] = DynamicScope::scopedvar_frame_max_count;
		limits[eDataScope_Frame_CarryOnPush] = DynamicScope::scopedvar_frame_max_count;
		limits[eDataScope_Frame_CarryOnPop] = DynamicScope::scopedvar_frame_max_count;
		limits[eDataScope_Frame_CarryOnBoth] = DynamicScope::scopedvar_frame_max_count;
		limits[eDataScope_Map] = 0;
		limits[eDataScope_MapEvent] = DynamicScope::scopedvar_mapevents_default_count;
	}

	template<DataScopeType S, typename Data_t> constexpr bool IsValid(limit_array_t const& limits, globals_array_t<Data_t> const& globals, const int id) {
		if constexpr (DynamicScope::IsGlobalScope(S)) {
			return id > 0 && id <= GetSizeWithLimit<S>(limits, globals);
		}
		if constexpr (DynamicScope::IsFrameScope(S)) {
			return id > 0 && id <= GetLimit<S>(limits);
		}
		if constexpr (DynamicScope::IsMapScope(S) || DynamicScope::IsMapEventScope(S)) {
			return id > 0 && id <= GetLimit<S>(limits);
		}
		return false;
	}

	template<DataScopeType S> constexpr size_t GetLimit(limit_array_t const& limits) {
		static_assert(S <= eDataScope_MAX);

		return limits[static_cast<int>(S)];
	}

	template<DataScopeType S> constexpr void SetLowerLimit(limit_array_t& limits, size_t limit) {
		static_assert(!DynamicScope::IsFrameScope(S), "Setting limit for Scope 'Frame' not allowed!");

		if constexpr (DynamicScope::IsGlobalScope(S)) {
			limits[static_cast<int>(S)] = limit;
		}
		if constexpr (DynamicScope::IsMapScope(S)) {
			if (limit > DynamicScope::scopedvar_maps_max_count) {
				Output::Debug("Invalid limit for Scope 'Map': {}", limit);
			}
			limits[static_cast<int>(S)] = std::min(limit, DynamicScope::scopedvar_maps_max_count);
		}
		if constexpr (DynamicScope::IsMapEventScope(S)) {
			if (limit > DynamicScope::scopedvar_maps_max_count) {
				Output::Debug("Invalid limit for Scope 'MapEvent': {}", limit);
			}
			limits[static_cast<int>(S)] = std::min(limit, DynamicScope::scopedvar_mapevents_max_count);
		}
	}

	template<DataScopeType S, typename Data_t> constexpr int GetSize(globals_array_t<Data_t> const& globals) {
		ASSERT_IS_GLOBAL_SCOPE(S, "GetSize");
		return static_cast<int>(globals[static_cast<int>(S)].size());
	}

	template<DataScopeType S, typename Data_t> constexpr int GetSizeWithLimit(limit_array_t const& limits, globals_array_t<Data_t> const& globals) {
		ASSERT_IS_GLOBAL_SCOPE(S, "GetSizeWithLimit");
		return std::max<int>(static_cast<int>(GetLimit<S>(limits)), static_cast<int>(globals[static_cast<int>(S)].size()));
	}

	template<DataScopeType S, typename Data_t> const Data_t& GetStorage(globals_array_t<Data_t> const& globals) {
		ASSERT_IS_GLOBAL_SCOPE(S, "GetStorage");
		return globals[static_cast<int>(S)];
	}

	template<DataScopeType S, typename Data_t> Data_t& GetStorageForEdit(globals_array_t<Data_t>& globals) {
		ASSERT_IS_GLOBAL_SCOPE(S, "GetStorageForEdit");
		return globals[static_cast<int>(S)];
	}
}

template <typename T, typename V>
class Game_VectorDataStorageBase {
public:
	typedef VarStorage::DataStorage_t<V, VarStorage::eStorageMode_Vector> Data_t;

	Game_VectorDataStorageBase() { DynamicScope::InitLimits(_limits); }

	SCOPED_WITH_DEFAULT inline bool IsValid(const int id) const { return DynamicScope::IsValid<S, Data_t>(_limits, _globals, id); }
	SCOPED_WITH_DEFAULT inline size_t GetLimit() const { return DynamicScope::GetLimit<S>(_limits); }
	SCOPED_WITH_DEFAULT inline void SetLowerLimit(size_t limit) { DynamicScope::SetLowerLimit<S>(_limits, limit); }

	SCOPED_GLOBAL_ONLY inline int GetSize() const { return DynamicScope::GetSize<S, Data_t>(_globals); }
	SCOPED_GLOBAL_ONLY inline int GetSizeWithLimit() const { return DynamicScope::GetSizeWithLimit<S, Data_t>(_limits, _globals); }

	template<DataScopeType = DataScopeType::eDataScope_Global, typename C = V>
	void SetData(std::vector<V> data);
	template<DataScopeType = DataScopeType::eDataScope_Global, typename C = V>
	std::vector<V> GetData() const;

protected:
	SCOPED_GLOBAL_ONLY const Data_t& GetStorage() const { return DynamicScope::GetStorage<S, Data_t>(_globals); }
	SCOPED_GLOBAL_ONLY Data_t& GetStorageForEdit() { return DynamicScope::GetStorageForEdit<S, Data_t>(_globals); }

private:
	DynamicScope::globals_array_t<Data_t> _globals;
	DynamicScope::limit_array_t _limits;
};

template <typename T, typename V>
class Game_MapDataStorageBase {
public:
	typedef VarStorage::DataStorage_t<V, VarStorage::eStorageMode_Map> Data_t;

	static_assert(!std::is_same<V, bool>::value);

	Game_MapDataStorageBase() { DynamicScope::InitLimits(_limits); }

protected:
	SCOPED_GLOBAL_ONLY const Data_t& GetStorage() const { return DynamicScope::GetStorage<S, Data_t>(_globals); }
	SCOPED_GLOBAL_ONLY Data_t& GetStorageForEdit() { return DynamicScope::GetStorageForEdit<S, Data_t>(_globals); }

private:
	DynamicScope::globals_array_t<Data_t> _globals;
	DynamicScope::limit_array_t _limits;
};

template <typename T, typename V>
class Game_MixedDataStorageBase {
public:
	typedef VarStorage::DataStorage_t<V, VarStorage::eStorageMode_Mixed> Data_t;

	Game_MixedDataStorageBase() { DynamicScope::InitLimits(_limits); }

	SCOPED_WITH_DEFAULT inline bool IsValid(const int id) const { return DynamicScope::IsValid<S, Data_t>(_limits, _globals, id); }
	SCOPED_WITH_DEFAULT inline size_t GetLimit() const { return DynamicScope::GetLimit<S>(_limits); }
	SCOPED_WITH_DEFAULT inline void SetLowerLimit(size_t limit) {
		DynamicScope::SetLowerLimit<S>(_limits, limit);
		for (int i = 0; i < _globals.size(); i++)
			_globals[i].bounds_vector = limit;
	}

	SCOPED_GLOBAL_ONLY inline int GetSize() const { return DynamicScope::GetSize<S, Data_t>(_globals); }
	SCOPED_GLOBAL_ONLY inline int GetSizeWithLimit() const { return DynamicScope::GetSizeWithLimit<S, Data_t>(_limits, _globals); }

	template<DataScopeType = DataScopeType::eDataScope_Global, typename C = V>
	void SetData(std::vector<V> data);
	template<DataScopeType = DataScopeType::eDataScope_Global, typename C = V>
	std::vector<V> GetData() const;

protected:
	SCOPED_GLOBAL_ONLY const Data_t& GetStorage() const { return DynamicScope::GetStorage<S, Data_t>(_globals); }
	SCOPED_GLOBAL_ONLY Data_t& GetStorageForEdit() { return DynamicScope::GetStorageForEdit<S, Data_t>(_globals); }

private:
	DynamicScope::globals_array_t<Data_t> _globals;
	DynamicScope::limit_array_t _limits;
};

template <typename T, typename V, VarStorage::DataStorageMode storage_mode>
using Game_DataStorageBase = std::conditional_t<storage_mode == VarStorage::eStorageMode_Vector,
	Game_VectorDataStorageBase<T, V>,
	std::conditional_t<storage_mode == VarStorage::eStorageMode_Map,
		Game_MapDataStorageBase<T, V>,
		Game_MixedDataStorageBase<T, V>
	>
>;

#define Game_DataStorage_Declare_TPL template <typename D, typename T, typename V, VarStorage::DataStorageMode storage_mode>
#define Game_DataStorage_TPL Game_DataStorage<D, T, V, storage_mode>

template <typename D, typename T, typename V, VarStorage::DataStorageMode storage_mode>
class Game_DataStorage : public Game_DataStorageBase<T, V, storage_mode> {
public:
	using BaseType = Game_DataStorageBase<T, V, storage_mode>;
	using typename Game_DataStorageBase<T, V, storage_mode>::Data_t;

	typedef DynamicScope::ScopedDataStorage<Data_t, V> ScopedDataStorage_t;
	typedef std::tuple<std::vector<V>&, std::vector<uint32_t>&, std::vector<uint32_t>&> FrameStorage_t;
	typedef std::tuple<const std::vector<V>&, const std::vector<uint32_t>&, const std::vector<uint32_t>&> FrameStorage_const_t;

	static constexpr int kMaxWarnings = 10;

	Game_DataStorage(int type);

	~Game_DataStorage() = default;

	virtual StringView GetName(int id, DataScopeType scope = DataScopeType::eDataScope_Global) const = 0;

	void SetWarning(int w);

	SCOPED_WITH_DEFAULT V Get(int id, Args... args) const;
	SCOPED_WITH_DEFAULT V Set(int id, V value, Args... args);
	SCOPED_WITH_DEFAULT void SetRange(int first_id, int last_id, V value, Args... args);

	void SetScopedStorageSaveData(std::vector<T> save);
	std::vector<T> GetScopedStorageSaveData() const;

	/* Facade declarations for DynamicScoped-only methods */
	SCOPED_DYN_ONLY	class dyn_scoped_facade_map_t {
	private:
		dyn_scoped_facade_map_t(Game_DataStorage_TPL& parent) : _parent(parent) { }

		Game_DataStorage_TPL& _parent;
		friend Game_DataStorage_TPL;
	public:
		bool IsStorageInitialized(int map_id) const;
		template<typename F>
		bool GetInherited(int id, int map_id, F&& get_parent_id, V& out_val) const;
		V GetDefaultValue(int id) const;
		bool IsDefined(int id, int map_id) const;
		bool IsReadOnly(int id, int map_id) const;
		bool IsAutoReset(int id, int map_id) const;
		bool IsDefaultValueDefined(int id, int map_id) const;
		bool IsInheritedValue(int id, int map_id) const;

		int CountElementsDefined(bool defined, int id) const;
		int CountElementsWithCondition(std::function<bool(const V)> op, int id) const;

		std::vector<std::tuple<int, int>> GetElementsDefined(bool defined, int max, int id) const;
		std::vector<std::tuple<int, int>> GetElementsWithCondition(std::function<bool(const V)> op, int max, int id) const;

		void ClearValue(int id, int map_id);
		void ResetTemporaryData(int map_id);
		void SetResetFlagForId(int id, bool reset_flag, int map_id);
	};
	SCOPED_DYN_ONLY	class dyn_scoped_facade_mapevent_t {
	private:
		dyn_scoped_facade_mapevent_t(Game_DataStorage_TPL& parent) : _parent(parent) { }

		Game_DataStorage_TPL& _parent;
		friend Game_DataStorage_TPL;
	public:
		bool IsStorageInitialized(int map_id, int event_id) const;
		V GetDefaultValue(int id) const;
		bool IsDefined(int id, int map_id, int event_id) const;
		bool IsReadOnly(int id, int map_id, int event_id) const;
		bool IsAutoReset(int id, int map_id, int event_id) const;
		bool IsDefaultValueDefined(int id, int map_id, int event_id) const;
		int CountElementsDefined(bool defined, int id, int map_id) const;
		int CountElementsWithCondition(std::function<bool(const V)> op, int id, int map_id) const;

		std::vector<std::tuple<int, int>> GetElementsDefined(bool defined, int max, int id, int map_id) const;
		std::vector<std::tuple<int, int>> GetElementsWithCondition(std::function<bool(const V)> op, int max, int id, int map_id) const;

		void ClearValue(int id, int map_id, int event_id);
		void ResetTemporaryData(int map_id, int event_id);
		void SetResetFlagForId(int id, bool reset_flag, int map_id, int event_id);
	};

	//These helper classes should not hold any state
	dyn_scoped_facade_map_t<DataScopeType::eDataScope_Map> scoped_map;
	dyn_scoped_facade_mapevent_t<DataScopeType::eDataScope_MapEvent> scoped_mapevent;
	/* end Facade declarations */
protected:
	using op_Func = V(*)(const V o, const V n);
	using value_Func = V(*)();

	SCOPED_GLOBAL_ONLY bool ShouldWarn(int first_id, int last_id) const;
	SCOPED_DYN_ONLY bool scopedShouldWarn(int first_id, int last_id, int map_id = 0, int event_id = 0) const;
	template<DataScopeType>
	void WarnGet(int id, int map_id, int event_id = 0) const;
	template<DataScopeType>
	void WarnSet(int id, V value, const char* op, int map_id, int event_id = 0) const;
	template<DataScopeType>
	void WarnSetRange(int first_id, int last_id, V value, const char* op, int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY bool scopedValidateReadOnly(int first_id, int last_id, int map_id = 0, int event_id = 0) const;

	template<DataScopeType> inline bool IsOfScope(int hash) const;
	template<DataScopeType> uint32_t MakeHash(int map_id, int event_id = 0) const;

	void IdsFromHash(uint32_t hash, DataScopeType& scope, int& map_id, int& event_id) const;

	virtual uint8_t scopedInitFlags(DataScopeType scope, int id) const = 0;
	virtual V scopedGetDefaultValue(DataScopeType scope, int id) const = 0;
	virtual void scopedGetDataFromSaveElement(T element, DataScopeType& scope, int& id, V& value, int& map_id, int& event_id, bool& reset_flag) const = 0;
	virtual T scopedCreateSaveElement(DataScopeType scope, int id, V value, int map_id, int event_id, bool reset_flag) const = 0;

	SCOPED_WITH_DEFAULT V PerformOperation(const int id, const V value, op_Func&& op, const char* warnOp, Args... args);

	SCOPED_WITH_DEFAULT void PrepareRange(const int first_id, const int last_id, Args... args);
	SCOPED_WITH_DEFAULT void PerformRangeOperation(const int first_id, const int last_id, const V value, op_Func&& op, Args... args);
	template<DataScopeType = DataScopeType::eDataScope_Global, typename F, typename... Args>
	void PerformRangeOperation(const int first_id, const int last_id, F&& value, op_Func&& op, Args... args);

	void AssignOp(V& target, V value) const;
	const FrameStorage_const_t GetFrameStorage(const lcf::rpg::SaveEventExecFrame* frame) const;
	FrameStorage_t GetFrameStorageForEdit(lcf::rpg::SaveEventExecFrame* frame);
	void SetCarryFlagForFrameStorage(std::vector<uint32_t>& carry_flags, int id) const;

	SCOPED_DYN_ONLY const ScopedDataStorage_t& GetScopedStorage(int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY ScopedDataStorage_t& GetScopedStorageForEdit(bool create, int map_id, int event_id = 0);

	template<DataScopeType>
	auto FormatLValue(int first_id, int last_id = 0, int map_id = 0, int event_id = 0) const;
	auto FormatRValue(V v, const char* operandType = nullptr, DataScopeType operandScope = DataScopeType::eDataScope_Global, int map_id = 0, int event_id = 0) const;

	int _type = -1;
	mutable int _warnings = kMaxWarnings;
private:
	using parent_id_Func = int(*)(const int id);

	/* DynamicScoped-only methods (hidden & only accessible through members of type 'dyn_scoped_facade_t' */
	SCOPED_DYN_ONLY bool scopedIsStorageInitialized(int map_id, int event_id = 0) const;
	SCOPED_MAP_ONLY bool scopedGetInherited(int id, int map_id, parent_id_Func&& get_parent_id, V& out_val) const;
	SCOPED_DYN_ONLY bool scopedIsDefined(int id, int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY bool scopedIsReadOnly(int id, int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY bool scopedIsDefaultValueDefined(int id, int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY bool scopedIsAutoReset(int id, int map_id, int event_id = 0) const;
	SCOPED_DYN_ONLY bool scopedIsInheritedValue(int id, int map_id) const;
	SCOPED_DYN_ONLY int scopedCountElementsDefined(bool defined, int id, int map_id = 0) const;
	SCOPED_DYN_ONLY int scopedCountElementsWithCondition(std::function<bool(const V)> op, int id, int map_id = 0) const;

	SCOPED_DYN_ONLY std::vector<std::tuple<int, int>> scopedGetElementsDefined(bool defined, int max, int id, int map_id = 0) const;
	SCOPED_DYN_ONLY std::vector<std::tuple<int, int>> scopedGetElementsWithCondition(std::function<bool(const V)> op, int max, int id, int map_id = 0) const;

	SCOPED_DYN_ONLY void scopedClearValue(int id, int map_id, int event_id = 0);
	SCOPED_DYN_ONLY void scopedResetTemporaryData(int map_id, int event_id = 0);
	SCOPED_DYN_ONLY	void scopedSetResetFlagForId(int id, bool reset_flag, int map_id, int event_id = 0);
	/* end DynamicScoped-only methods */

	V _defaultValue = 0;
	std::unordered_map<uint32_t, ScopedDataStorage_t> _scopedData;
	ScopedDataStorage_t _scoped_null_storage;
};

// Game_VectorDataStorageBase definitions

template <typename T, typename V>
template<DataScopeType S, typename C>
inline void Game_VectorDataStorageBase<T, V>::SetData(std::vector<V> data) {
	ASSERT_IS_GLOBAL_SCOPE(S, "SetData");

	static_assert(static_cast<int>(S) < DynamicScope::count_global_scopes);

	if constexpr (std::is_same<C, V>::value) {
		_globals[static_cast<int>(S)].setData(std::move(data));
	} else {
		std::vector<V> data_conv;
		data_conv.resize(data.size());
		for (int i = 0; i < data.size(); i++)
			data_conv = static_cast<V>(data[i]);
		_globals[static_cast<int>(S)].setData(std::move(data_conv));
	}
}

template <typename T, typename V>
template<DataScopeType S, typename C>
inline std::vector<V> Game_VectorDataStorageBase<T, V>::GetData() const {
	ASSERT_IS_GLOBAL_SCOPE(S, "GetData");

	static_assert(static_cast<int>(S) < DynamicScope::count_global_scopes);

	if constexpr (std::is_same<C, V>::value) {
		return _globals[static_cast<int>(S)].getData();
	} else {
		auto data = _globals[static_cast<int>(S)].getData();
		auto data_conv = std::vector<C>(data.size());
		for (int i = 0; i < data.size(); i++)
			data[i] = static_cast<C>(data[i]);
		return data;
	}
}

// END Game_VectorDataStorageBase definitions


// Game_MixedDataStorageBase definitions

template <typename T, typename V>
template<DataScopeType S, typename C>
inline void Game_MixedDataStorageBase<T, V>::SetData(std::vector<V> data) {
	ASSERT_IS_GLOBAL_SCOPE(S, "SetData");

	static_assert(static_cast<int>(S) < DynamicScope::count_global_scopes);

	if constexpr (std::is_same<C, V>::value) {
		_globals[static_cast<int>(S)].setData(std::move(data));
	} else {
		std::vector<V> data_conv;
		data_conv.resize(data.size());
		for (int i = 0; i < data.size(); i++)
			data_conv = static_cast<V>(data[i]);
		_globals[static_cast<int>(S)].setData(std::move(data_conv));
	}
}

template <typename T, typename V>
template<DataScopeType S, typename C>
inline std::vector<V> Game_MixedDataStorageBase<T, V>::GetData() const {
	ASSERT_IS_GLOBAL_SCOPE(S, "GetData");

	static_assert(static_cast<int>(S) < DynamicScope::count_global_scopes);

	if constexpr (std::is_same<C, V>::value) {
		return _globals[static_cast<int>(S)].getData();
	} else {
		auto data = _globals[static_cast<int>(S)].getData();
		auto data_conv = std::vector<C>(data.size());
		for (int i = 0; i < data.size(); i++)
			data[i] = static_cast<C>(data[i]);
		return data;
	}
}

// END Game_MixedDataStorageBase definitions

Game_DataStorage_Declare_TPL
inline Game_DataStorage_TPL::Game_DataStorage(int type)
	: Game_DataStorageBase<T, V, storage_mode>(), _type(type), scoped_map(*this), scoped_mapevent(*this) {
	_scoped_null_storage = ScopedDataStorage_t();
	_scoped_null_storage.valid = false;
}

Game_DataStorage_Declare_TPL
inline void Game_DataStorage_TPL::SetWarning(int w) {
	_warnings = w;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::ShouldWarn(int first_id, int last_id) const {
	ASSERT_IS_GLOBAL_SCOPE(S, "ShouldWarn");

	if constexpr (storage_mode == VarStorage::eStorageMode_Vector) {
		return (first_id <= 0 || last_id > this->template GetSizeWithLimit<S>()) && (_warnings > 0);
	}
	return (first_id <= 0  && _warnings > 0);
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::scopedShouldWarn(int first_id, int last_id, int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "scopedShouldWarn");

	if constexpr (DynamicScope::IsFrameScope(S)) {
		return first_id <= 0 || last_id > this->template GetLimit<S>();
	} else if constexpr (storage_mode == VarStorage::eStorageMode_Vector) {
		if constexpr (DynamicScope::IsMapScope(S)) {
			return (map_id <= 0 || first_id <= 0 || last_id > this->template GetLimit<S>()) && (_warnings > 0);
		}
		if constexpr (DynamicScope::IsMapEventScope(S)) {
			return (map_id <= 0 || event_id <= 0 || first_id <= 0 || last_id > this->template GetLimit<S>()) && (_warnings > 0);
		}
	} else {
		if constexpr (DynamicScope::IsMapScope(S)) {
			return (map_id <= 0 || first_id <= 0) && (_warnings > 0);
		}
		if constexpr (DynamicScope::IsMapEventScope(S)) {
			return (map_id <= 0 || event_id <= 0 || first_id <= 0) && (_warnings > 0);
		}
	}
	return false;
}


Game_DataStorage_Declare_TPL
template<DataScopeType S, typename F, typename... Args>
void Game_DataStorage_TPL::PerformRangeOperation(const int first_id, const int last_id, F&& value, op_Func&& op, Args... args) {

	if constexpr (sizeof...(Args) == 0) {
		ASSERT_IS_GLOBAL_SCOPE(S, "PerformRangeOperation");

		auto& storage = this->template GetStorageForEdit<S>();
		for (int i = std::max(1, first_id); i <= last_id; ++i) {
			if constexpr (std::is_same<V, bool>::value) {
				bool v = storage[i];
				AssignOp(v, op(v, value()));
				storage[i] = v;
			} else {
				V& v = storage[i];
				AssignOp(v, op(v, value()));
			}
		}
	} else if constexpr (DynamicScope::IsFrameScope(S)) {
		constexpr bool carry_in = (S == eDataScope_Frame_CarryOnPush || S == eDataScope_Frame_CarryOnBoth);
		constexpr bool carry_out = (S == eDataScope_Frame_CarryOnPop || S == eDataScope_Frame_CarryOnBoth);

		auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorageForEdit(args...);
		int hardcapped_last_id = std::min(last_id, static_cast<int>(this->template GetLimit<S>()));

		for (int i = std::max(0, first_id); i < hardcapped_last_id; ++i) {
			if constexpr (carry_in) {
				SetCarryFlagForFrameStorage(carry_flags_in, i + 1);
			}
			if constexpr (carry_out) {
				SetCarryFlagForFrameStorage(carry_flags_out, i + 1);
			}

			if constexpr (std::is_same<V, bool>::value) {
				bool v = vec[i];
				AssignOp(v, op(v, value()));
				vec[i] = v;
			} else {
				V& v = vec[i];
				AssignOp(v, op(v, value()));
			}
		}
	} else {
		ASSERT_IS_VARIABLE_SCOPE(S, "PerformRangeOperation");

		auto& storage = this->template GetScopedStorageForEdit<S>(true, args...);
		for (int i = std::max(1, first_id); i <= last_id; ++i) {
			if constexpr (std::is_same<V, bool>::value) {
				bool v = storage[i];
				AssignOp(v, op(v, value()));
				storage[i] = v;
			} else {
				V& v = storage[i];
				AssignOp(v, op(v, value()));
			}
			storage.flags[i - 1] |= ScopedDataStorage_t::eFlag_ValueDefined;
		}
	}
}

Game_DataStorage_Declare_TPL
void Game_DataStorage_TPL::SetCarryFlagForFrameStorage(std::vector<uint32_t>& carry_flags, int id) const {
	const int idx_mask = (id - 1) / 32;
	if (EP_UNLIKELY(idx_mask >= static_cast<int>(carry_flags.size()))) {
		carry_flags.resize(idx_mask + 1, 0);
	}
	carry_flags[idx_mask] |= (1 << ((id - 1) % 32));
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::scopedValidateReadOnly(int first_id, int last_id, int map_id, int event_id) const {
	if (first_id == last_id) {
		if (scopedIsReadOnly<S>(first_id, map_id, event_id)) {
			Output::Debug("Invalid write to {}! (Set as Read-Only)", this->FormatLValue<S>(first_id, 0, map_id, event_id));
			return false;
		}
	} else {
		if (last_id < first_id) {
			std::swap(first_id, last_id);
		}
		for (int id = first_id; id <= last_id; id++) {
			if (scopedIsReadOnly<S>(id, map_id, event_id)) {
				Output::Debug("Invalid write to {}! ('{}' is set as Read-Only)", this->FormatLValue<S>(first_id, last_id, map_id, event_id), id);
				return false;
			}
		}
	}
	return true;
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline const DynamicScope::ScopedDataStorage<typename Game_DataStorageBase<T, V, storage_mode>::Data_t, V>& Game_DataStorage_TPL::GetScopedStorage(int map_id, int event_id) const {
	ASSERT_IS_VARIABLE_SCOPE(S, "GetScopedStorage");

	int32_t hash = MakeHash<S>(map_id, event_id);
	auto it = _scopedData.find(hash);

	if (it == _scopedData.end()) {
		return {};
	}
	return it->second;
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline DynamicScope::ScopedDataStorage<typename Game_DataStorageBase<T, V, storage_mode>::Data_t, V>& Game_DataStorage_TPL::GetScopedStorageForEdit(bool create, int map_id, int event_id) {
	ASSERT_IS_VARIABLE_SCOPE(S, "GetScopedStorageForEdit");

	int32_t hash = MakeHash<S>(map_id, event_id);
	auto it = _scopedData.find(hash);
	ScopedDataStorage_t storage;

	if (it != _scopedData.end())
		return it->second;

	if (!create) {
		return _scoped_null_storage;
	}

	storage = ScopedDataStorage_t();
	storage.valid = true;
	storage.map_id = map_id;
	storage.evt_id = event_id;

	_scopedData[hash] = storage;
	
	return _scopedData[hash];
}

Game_DataStorage_Declare_TPL
SCOPED_IMPL inline V Game_DataStorage_TPL::Get(int id, Args... args) const {
	if constexpr (sizeof...(Args) == 0) {
		ASSERT_IS_GLOBAL_SCOPE(S, "Get");

		if (EP_UNLIKELY(ShouldWarn<S>(id, id))) {
			WarnGet<S>(id, 0, 0);
		}
		const Data_t& storage = this->template GetStorage<S>();
		if (!storage.containsKey(id))
			return false;
		return storage[id];
	} else if constexpr(DynamicScope::IsFrameScope(S)) {

		if (EP_UNLIKELY(scopedShouldWarn<S>(id, id, 0, 0))) {
			WarnGet<S>(id, 0, 0);
		}

		const auto [vec, carry_flags_in, carry_flags_out] = GetFrameStorage(args...);
		if (id <= 0 || id > static_cast<int>(vec.size())) {
			return false;
		}
		return vec[id - 1];
	} else {
		ASSERT_IS_VARIABLE_SCOPE(S, "Get");

		if (EP_UNLIKELY(scopedShouldWarn<S>(id, id, args...))) {
			WarnGet<S>(id, args...);
		}

		const ScopedDataStorage_t& storage = GetScopedStorage<S>(args...);
		if (!storage.valid || !storage.containsKey(id)) {
			return false;
		}
		return storage[id];
	}
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline auto Game_DataStorage_TPL::FormatLValue(int first_id, int last_id, int map_id, int event_id) const {
	if constexpr (DynamicScope::IsGlobalScope(S) || DynamicScope::IsFrameScope(S)) {
		return last_id == 0
			? fmt::format("{}{}[{}]", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id)
			: fmt::format("{}{}[{},{}]", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id, last_id);
	}
	if constexpr (DynamicScope::IsMapScope(S)) {
		return last_id == 0
			? fmt::format("{}{}[{}]{{M{}}}", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id, map_id)
			: fmt::format("{}{}[{},{}]{{M{}}}", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id, last_id, map_id);
	}
	if constexpr (DynamicScope::IsMapEventScope(S)) {
		return last_id == 0
			? fmt::format("{}{}[{}]{{M{},E{}}}", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id, map_id, event_id)
			: fmt::format("{}{}[{},{}]{{M{},E{]}}", DynamicScope::ScopeToStr(S), VarStorage::TypeToStr(_type), first_id, last_id, map_id, event_id);
	}
}

Game_DataStorage_Declare_TPL
inline auto Game_DataStorage_TPL::FormatRValue(V v, const char* operandType, DataScopeType operandScope, int map_id, int event_id) const {

	//special handling for string storage
	if constexpr (std::is_same<V, std::string>::value) {
		auto str = static_cast<std::string>(v);
		if (str.length() > 8)
			v = str.substr(0, 5) + "...";
	}

	if (operandType == nullptr) {
		return fmt::format("{}", v);
	}
	if (DynamicScope::IsGlobalScope(operandScope) || DynamicScope::IsFrameScope(operandScope)) {
		return fmt::format("{}{}[{}]", DynamicScope::ScopeToStr(operandScope), operandType, v);
	}
	if (DynamicScope::IsMapScope(operandScope)) {
		return fmt::format("{}{}[{}]{{M{}}}", DynamicScope::ScopeToStr(operandScope), operandType, v, map_id);
	}
	if (DynamicScope::IsMapEventScope(operandScope)) {
		return fmt::format("{}{}[{}]{{M{},E{}}}", DynamicScope::ScopeToStr(operandScope), operandType, v, map_id, event_id);
	}
	return std::string();
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline void Game_DataStorage_TPL::WarnGet(int id, int map_id, int event_id) const {
	Output::Debug("Invalid read {}!", FormatLValue<S>(id, 0, map_id, event_id));
	--_warnings;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline void Game_DataStorage_TPL::WarnSet(int id, V value, const char* op, int map_id, int event_id) const {
	Output::Debug("Invalid write {} {} {}!", FormatLValue<S>(id, 0, map_id, event_id), op, FormatRValue(value));
	--_warnings;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline void Game_DataStorage_TPL::WarnSetRange(int first_id, int last_id, V value, const char* op, int map_id, int event_id) const {
	Output::Debug("Invalid write {} {} {}!", FormatLValue<S>(first_id, last_id, map_id, event_id), op, FormatRValue(value));
	--_warnings;
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline uint32_t Game_DataStorage_TPL::MakeHash(int map_id, int event_id) const {
	return (static_cast<uint32_t>(S) + static_cast<uint32_t>(map_id) << 4 + static_cast<uint32_t>(event_id) << 16);
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::IsOfScope(int hash) const {
	return (hash & 0xF) == static_cast<uint32_t>(S);
}

Game_DataStorage_Declare_TPL
inline void Game_DataStorage_TPL::IdsFromHash(uint32_t hash, DataScopeType& scope, int& map_id, int& event_id) const {
	scope = static_cast<DataScopeType>(hash & 0xF);
	map_id = (hash >> 4) & 0xFFFF;
	event_id = (hash >> 16) & 0xFFFF;
}

/* Facade implementation for DynamicScoped-only methods */

Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsStorageInitialized(int map_id) const {
	return _parent.scopedIsStorageInitialized<S>(map_id, 0);
}

Game_DataStorage_Declare_TPL
template<DataScopeType S>
template<typename F>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::GetInherited(int id, int map_id, F&& get_parent_id, V& out_val) const {
	return _parent.scopedGetInherited<S>(id, map_id, get_parent_id, out_val);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline V Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::GetDefaultValue(int id) const {
	return _parent.scopedGetDefaultValue(S, id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsDefined(int id, int map_id) const {
	return _parent.scopedIsDefined<S>(id, map_id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsReadOnly(int id, int map_id) const {
	return _parent.scopedIsReadOnly<S>(id, map_id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsAutoReset(int id, int map_id) const {
	return _parent.scopedIsAutoReset<S>(id, map_id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsDefaultValueDefined(int id, int map_id) const {
	return _parent.scopedIsDefaultValueDefined<S>(id, map_id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::IsInheritedValue(int id, int map_id) const {
	return _parent.scopedIsInheritedValue<S>(id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline int Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::CountElementsDefined(bool defined, int id) const {
	return _parent.scopedCountElementsDefined<S>(defined, id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline int Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::CountElementsWithCondition(std::function<bool(const V)> op, int id) const {
	return _parent.scopedCountElementsWithCondition<S>(op, id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline std::vector<std::tuple<int, int>> Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::GetElementsDefined(bool defined, int max, int id) const {
	return _parent.scopedGetElementsDefined<S>(defined, max, id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline std::vector<std::tuple<int, int>> Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::GetElementsWithCondition(std::function<bool(const V)> op, int max, int id) const {
	return _parent.scopedGetElementsWithCondition<S>(op, max, id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::ClearValue(int id, int map_id) {
	return _parent.scopedClearValue<S>(id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::ResetTemporaryData(int map_id) {
	return _parent.scopedResetTemporaryData<S>(map_id, 0);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_map_t<S>::SetResetFlagForId(int id, bool reset_flag, int map_id) {
	return _parent.scopedSetResetFlagForId<S>(id, reset_flag, map_id, 0);
}


Game_DataStorage_Declare_TPL
template<DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::IsStorageInitialized(int map_id, int event_id) const {
	return _parent.scopedIsStorageInitialized<S>(map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline V Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::GetDefaultValue(int id) const {
	return _parent.scopedGetDefaultValue(S, id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::IsDefined(int id, int map_id, int event_id) const {
	return _parent.scopedIsDefined<S>(id, map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::IsReadOnly(int id, int map_id, int event_id) const {
	return _parent.scopedIsReadOnly<S>(id, map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::IsAutoReset(int id, int map_id, int event_id) const {
	return _parent.scopedIsAutoReset<S>(id, map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline bool Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::IsDefaultValueDefined(int id, int map_id, int event_id) const {
	return _parent.scopedIsDefaultValueDefined<S>(id, map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline int Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::CountElementsDefined(bool defined, int id, int map_id) const {
	return _parent.scopedCountElementsDefined<S>(defined, id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline int Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::CountElementsWithCondition(std::function<bool(const V)> op, int id, int map_id) const {
	return _parent.scopedCountElementsWithCondition<S>(op, id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline std::vector<std::tuple<int, int>> Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::GetElementsDefined(bool defined, int max, int id, int map_id) const {
	return _parent.scopedGetElementsDefined<S>(defined, max, id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline std::vector<std::tuple<int, int>> Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::GetElementsWithCondition(std::function<bool(const V)> op, int max, int id, int map_id) const {
	return _parent.scopedGetElementsWithCondition<S>(op, max, id, map_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::ClearValue(int id, int map_id, int event_id) {
	return _parent.scopedClearValue<S>(id, map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::ResetTemporaryData(int map_id, int event_id) {
	return _parent.scopedResetTemporaryData<S>(map_id, event_id);
}

Game_DataStorage_Declare_TPL
template <DataScopeType S>
inline void Game_DataStorage_TPL::dyn_scoped_facade_mapevent_t<S>::SetResetFlagForId(int id, bool reset_flag, int map_id, int event_id) {
	return _parent.scopedSetResetFlagForId<S>(id, reset_flag, map_id, event_id);
}

/* Facade implementation end */

/* CRTP for inlining the otherwise 'virtual' member functions "AssignOp" & "GetFrameStorage" */

class Game_SwitchesBase : public Game_DataStorage<Game_SwitchesBase, lcf::rpg::SaveScopedSwitchData, game_bool, VarStorage::eStorageMode_Vector> {
public:
	using Var_t = game_bool;

	inline Game_SwitchesBase()
		: Game_DataStorage<Game_SwitchesBase, lcf::rpg::SaveScopedSwitchData, game_bool, VarStorage::eStorageMode_Vector>(VarStorage::DataStorageType::eStorageType_Variable) {
	}

private:
	void AssignOpImpl(game_bool& target, game_bool value) const;

	const FrameStorage_const_t GetFrameStorageImpl(const lcf::rpg::SaveEventExecFrame* frame) const;
	FrameStorage_t GetFrameStorageForEditImpl(lcf::rpg::SaveEventExecFrame* frame);

	friend class Game_DataStorage<Game_SwitchesBase, lcf::rpg::SaveScopedSwitchData, game_bool, VarStorage::eStorageMode_Vector>;
};

class Game_VariablesBase : public Game_DataStorage<Game_VariablesBase, lcf::rpg::SaveScopedVariableData, int32_t, VarStorage::eStorageMode_Vector> {
public:
	using Var_t = int32_t;

	inline Game_VariablesBase(Var_t minval, Var_t maxval)
		: Game_DataStorage<Game_VariablesBase, lcf::rpg::SaveScopedVariableData, int32_t, VarStorage::eStorageMode_Vector>(VarStorage::DataStorageType::eStorageType_Variable),
		_min(minval), _max(maxval) {
		if (minval >= maxval) {
			Output::Error("Variables: Invalid var range: [{}, {}]", minval, maxval);
		}
	}

	inline Var_t GetMaxValue() const { return _max; };
	inline Var_t GetMinValue() const {	return _min; }

private:
	void AssignOpImpl(Var_t& target, Var_t value) const;

	const FrameStorage_const_t GetFrameStorageImpl(const lcf::rpg::SaveEventExecFrame* frame) const;
	FrameStorage_t GetFrameStorageForEditImpl(lcf::rpg::SaveEventExecFrame* frame);

	Var_t _min = 0;
	Var_t _max = 0;

	friend class Game_DataStorage<Game_VariablesBase, lcf::rpg::SaveScopedVariableData, int32_t, VarStorage::eStorageMode_Vector>;
};

template class Game_DataStorage<Game_SwitchesBase, lcf::rpg::SaveScopedSwitchData, game_bool, VarStorage::eStorageMode_Vector>;
template class Game_DataStorage<Game_VariablesBase, lcf::rpg::SaveScopedVariableData, int32_t, VarStorage::eStorageMode_Vector>;

Game_DataStorage_Declare_TPL
inline void Game_DataStorage_TPL::AssignOp(V& target, V value) const {
	reinterpret_cast< const D&>(*this).AssignOpImpl(target, value);
}

Game_DataStorage_Declare_TPL
inline const typename Game_DataStorage_TPL::FrameStorage_const_t Game_DataStorage_TPL::GetFrameStorage(const lcf::rpg::SaveEventExecFrame* frame) const {
	return reinterpret_cast<const D&>(*this).GetFrameStorage(frame);
}

Game_DataStorage_Declare_TPL
inline typename Game_DataStorage_TPL::FrameStorage_t Game_DataStorage_TPL::GetFrameStorageForEdit(lcf::rpg::SaveEventExecFrame* frame) {
#ifndef SCOPEDVARS_LIBLCF_STUB
	frame->easyrpg_framevars_in_use = true;
#endif
	return reinterpret_cast<D&>(*this).GetFrameStorageForEdit(frame);
}

#endif
