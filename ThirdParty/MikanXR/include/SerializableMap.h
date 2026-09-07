#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableMap.rfkh.h"
#endif

#include <memory>
#include <utility>
#include <cstddef>

namespace Serialization NAMESPACE()
{
class SERIALIZATION_API CLASS() IMapConstEnumerator
{
public:
	virtual ~IMapConstEnumerator()= default;

	virtual void reset()= 0;
	virtual bool isValid() const= 0;
	virtual void next()= 0;
	virtual const void* getKeyRaw() const= 0;
	virtual const void* getValueRaw() const= 0;

#ifdef SERIALIZATION_REFLECTION_ENABLED
	Serialization_IMapConstEnumerator_GENERATED
#endif
};

class SERIALIZATION_API CLASS() IMapEnumerator
{
public:
	virtual ~IMapEnumerator()= default;

	virtual void reset()= 0;
	virtual bool isValid() const= 0;
	virtual void next()= 0;
	virtual void* getKeyRaw()= 0;
	virtual void* getValueRaw()= 0;

#ifdef SERIALIZATION_REFLECTION_ENABLED
	Serialization_IMapEnumerator_GENERATED
#endif
};

template <typename t_key, typename t_value>
class Map;

// C4251: MapConstEnumerator, MapEnumerator, and Map members are all primitive types
// (pointer + size_t), layout-safe across DLL boundaries — false positive for these
// custom containers.
#pragma warning(push)
#pragma warning(disable : 4251)

// MapConstEnumerator uses an index into the flat entry array.
// Layout: {const Map*, size_t} = 16 bytes on x64, identical Debug/Release.
template <typename t_key, typename t_value>
class CLASS() MapConstEnumerator : public IMapConstEnumerator
{
public:
	MapConstEnumerator(const Map<t_key, t_value>& map)
		: m_map(&map)
		, m_index(0)
	{
	}

	virtual void reset() override { m_index= 0; }

	virtual bool isValid() const override { return m_index < m_map->size(); }

	virtual void next() override
	{
		if (isValid())
			++m_index;
	}

	virtual const void* getKeyRaw() const override
	{
		const auto* entry= m_map->getEntryAt(m_index);
		return entry ? &entry->key : nullptr;
	}

	virtual const void* getValueRaw() const override
	{
		const auto* entry= m_map->getEntryAt(m_index);
		return entry ? &entry->value : nullptr;
	}

#ifdef SERIALIZATION_REFLECTION_ENABLED
Serialization_MapConstEnumerator_GENERATED
#endif

	private : const Map<t_key, t_value>* m_map;
	std::size_t m_index;
};

// MapEnumerator uses an index into the flat entry array.
// Layout: {Map*, size_t} = 16 bytes on x64, identical Debug/Release.
template <typename t_key, typename t_value>
class CLASS() MapEnumerator : public IMapEnumerator
{
public:
	MapEnumerator(Map<t_key, t_value>& map)
		: m_map(&map)
		, m_index(0)
	{
	}

	virtual void reset() override { m_index= 0; }

	virtual bool isValid() const override { return m_index < m_map->size(); }

	virtual void next() override
	{
		if (isValid())
			++m_index;
	}

	virtual void* getKeyRaw() override
	{
		auto* entry= m_map->getEntryAtMutable(m_index);
		return entry ? (void*)&entry->key : nullptr;
	}

	virtual void* getValueRaw() override
	{
		auto* entry= m_map->getEntryAtMutable(m_index);
		return entry ? (void*)&entry->value : nullptr;
	}

#ifdef SERIALIZATION_REFLECTION_ENABLED
Serialization_MapEnumerator_GENERATED
#endif

	private : Map<t_key, t_value>* m_map;
	std::size_t m_index;
};

// Map<K,V> uses a plain flat-array layout: {Entry*, size_t, size_t} = 24 bytes on x64,
// identical in Debug and Release builds (no std::map _Container_proxy overhead).
// Safe to use across DLL boundaries built with different MSVC CRT configurations.
// Entry order matches insertion order; lookup is O(n) linear scan (suitable for small maps).
template <typename t_key, typename t_value>
class CLASS() Map
{
public:
	struct Entry
	{
		t_key key;
		t_value value;
	};

	Map() noexcept
		: m_entries(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
	}

	Map(const Map& other)
		: m_entries(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
		reserve(other.m_size);
		for (std::size_t i= 0; i < other.m_size; ++i)
		{
			m_entries[m_size]= other.m_entries[i];
			++m_size;
		}
	}

	Map(Map&& other) noexcept
		: m_entries(other.m_entries)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
	{
		other.m_entries= nullptr;
		other.m_size= 0;
		other.m_capacity= 0;
	}

	~Map() { delete[] m_entries; }

	Map& operator=(const Map& other)
	{
		if (this != &other)
		{
			m_size= 0;
			reserve(other.m_size);
			for (std::size_t i= 0; i < other.m_size; ++i)
			{
				m_entries[m_size]= other.m_entries[i];
				++m_size;
			}
		}
		return *this;
	}

	Map& operator=(Map&& other) noexcept
	{
		if (this != &other)
		{
			delete[] m_entries;
			m_entries= other.m_entries;
			m_size= other.m_size;
			m_capacity= other.m_capacity;
			other.m_entries= nullptr;
			other.m_size= 0;
			other.m_capacity= 0;
		}
		return *this;
	}

	METHOD() void clear() noexcept { m_size= 0; }

	METHOD() std::size_t size() const noexcept { return m_size; }

	METHOD() std::shared_ptr<IMapEnumerator> getEnumerator()
	{
		return std::make_shared<MapEnumerator<t_key, t_value>>(*this);
	}

	METHOD() std::shared_ptr<IMapConstEnumerator> getConstEnumerator() const
	{
		return std::make_shared<MapConstEnumerator<t_key, t_value>>(*this);
	}

	METHOD() const void* getOrAddRawValueMutable(const t_key& key)
	{
		std::size_t idx= findIndex(key);
		if (idx < m_size)
			return &m_entries[idx].value;
		if (m_size == m_capacity)
			grow();
		m_entries[m_size].key= key;
		return &m_entries[m_size++].value;
	}

	METHOD() const void* getRawValue(const t_key& key) const
	{
		std::size_t idx= findIndex(key);
		return idx < m_size ? &m_entries[idx].value : nullptr;
	}

	METHOD() void* getRawValueMutable(const t_key& key) { return const_cast<void*>(getRawValue(key)); }

	t_value& operator[](const t_key& key)
	{
		std::size_t idx= findIndex(key);
		if (idx < m_size)
			return m_entries[idx].value;
		if (m_size == m_capacity)
			grow();
		m_entries[m_size].key= key;
		return m_entries[m_size++].value;
	}

	void insert(std::pair<t_key, t_value> pair)
	{
		std::size_t idx= findIndex(pair.first);
		if (idx < m_size)
		{
			m_entries[idx].value= std::move(pair.second);
		}
		else
		{
			if (m_size == m_capacity)
				grow();
			m_entries[m_size].key= std::move(pair.first);
			m_entries[m_size].value= std::move(pair.second);
			++m_size;
		}
	}

	// Entry access for iterating and for MapEnumerator
	const Entry* getEntryAt(std::size_t index) const { return index < m_size ? &m_entries[index] : nullptr; }

	Entry* getEntryAtMutable(std::size_t index) { return index < m_size ? &m_entries[index] : nullptr; }

	// Range iteration (for range-based for and DLL-internal copy loops)
	Entry* begin() noexcept { return m_entries; }
	const Entry* begin() const noexcept { return m_entries; }
	Entry* end() noexcept { return m_entries + m_size; }
	const Entry* end() const noexcept { return m_entries + m_size; }

	// Typed find — returns pointer to value or nullptr if not found.
	// Replaces std::map::find() for callers that need a typed result.
	const t_value* findValue(const t_key& key) const
	{
		std::size_t idx= findIndex(key);
		return idx < m_size ? &m_entries[idx].value : nullptr;
	}

	t_value* findValueMutable(const t_key& key)
	{
		std::size_t idx= findIndex(key);
		return idx < m_size ? &m_entries[idx].value : nullptr;
	}

	bool contains(const t_key& key) const { return findIndex(key) < m_size; }

	bool empty() const noexcept { return m_size == 0; }

#ifdef SERIALIZATION_REFLECTION_ENABLED
Serialization_Map_GENERATED
#endif

	private : std::size_t
			  findIndex(const t_key& key) const
	{
		for (std::size_t i= 0; i < m_size; ++i)
		{
			if (m_entries[i].key == key)
				return i;
		}
		return m_size;
	}

	void grow() { reserve(m_capacity == 0 ? 4 : m_capacity * 2); }

	void reserve(std::size_t newCapacity)
	{
		if (newCapacity <= m_capacity)
			return;
		Entry* newEntries= new Entry[newCapacity]();
		for (std::size_t i= 0; i < m_size; ++i)
			newEntries[i]= std::move(m_entries[i]);
		delete[] m_entries;
		m_entries= newEntries;
		m_capacity= newCapacity;
	}

	Entry* m_entries;
	std::size_t m_size;
	std::size_t m_capacity;
};
#pragma warning(pop)
}; // namespace Serialization

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableMap_GENERATED
#endif
