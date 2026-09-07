#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableList.rfkh.h"
#endif

#include <initializer_list>
#include <utility>
#include <cstddef>

namespace Serialization NAMESPACE()
{
// C4251: List<T> members are all primitive types (pointer + size_t), layout-safe
// across DLL boundaries — the warning is a false positive for this custom container.
#pragma warning(push)
#pragma warning(disable : 4251)
// List<T> uses a plain flat-array layout: {T*, size_t, size_t} = 24 bytes on x64,
// identical in Debug and Release builds (no std::vector _Container_proxy overhead).
// Safe to use across DLL boundaries built with different MSVC CRT configurations.
template <class T>
class CLASS() List
{
public:
	List() noexcept
		: m_data(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
	}

	List(std::initializer_list<T> init)
		: m_data(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
		reserve(init.size());
		for (const T& item : init)
			push_back(item);
	}

	List(const List& other)
		: m_data(nullptr)
		, m_size(0)
		, m_capacity(0)
	{
		reserve(other.m_size);
		for (std::size_t i= 0; i < other.m_size; ++i)
			push_back(other.m_data[i]);
	}

	List(List&& other) noexcept
		: m_data(other.m_data)
		, m_size(other.m_size)
		, m_capacity(other.m_capacity)
	{
		other.m_data= nullptr;
		other.m_size= 0;
		other.m_capacity= 0;
	}

	~List() { delete[] m_data; }

	List& operator=(const List& other)
	{
		if (this != &other)
		{
			m_size= 0;
			reserve(other.m_size);
			for (std::size_t i= 0; i < other.m_size; ++i)
				push_back(other.m_data[i]);
		}
		return *this;
	}

	List& operator=(List&& other) noexcept
	{
		if (this != &other)
		{
			delete[] m_data;
			m_data= other.m_data;
			m_size= other.m_size;
			m_capacity= other.m_capacity;
			other.m_data= nullptr;
			other.m_size= 0;
			other.m_capacity= 0;
		}
		return *this;
	}

	METHOD() std::size_t size() const noexcept { return m_size; }

	METHOD() void resize(const std::size_t& newSize) noexcept
	{
		if (newSize > m_capacity)
			reserve(newSize);
		m_size= newSize;
	}

	METHOD() const void* getRawElement(const std::size_t& index) const
	{
		if (index < m_size)
			return &m_data[index];
		return nullptr;
	}

	METHOD() void* getRawElementMutable(const std::size_t& index) { return const_cast<void*>(getRawElement(index)); }

	void push_back(const T& value)
	{
		if (m_size == m_capacity)
			reserve(m_capacity == 0 ? 4 : m_capacity * 2);
		m_data[m_size++]= value;
	}

	T& operator[](std::size_t index) { return m_data[index]; }
	const T& operator[](std::size_t index) const { return m_data[index]; }

	T* begin() noexcept { return m_data; }
	const T* begin() const noexcept { return m_data; }
	T* end() noexcept { return m_data + m_size; }
	const T* end() const noexcept { return m_data + m_size; }
	T* data() noexcept { return m_data; }
	const T* data() const noexcept { return m_data; }

	void clear() noexcept { m_size= 0; }

	void assign(const T* first, const T* last)
	{
		const std::size_t count= static_cast<std::size_t>(last - first);
		if (count > m_capacity)
			reserve(count);
		for (std::size_t i= 0; i < count; ++i)
			m_data[i]= first[i];
		m_size= count;
	}

	bool empty() const noexcept { return m_size == 0; }

#ifdef SERIALIZATION_REFLECTION_ENABLED
Serialization_List_GENERATED
#endif

	private : void
			  reserve(std::size_t newCapacity)
	{
		if (newCapacity <= m_capacity)
			return;
		// Value-initialize the new array so primitive types (int, float) start at zero,
		// matching std::vector<T>::resize() behavior.
		T* newData= new T[newCapacity]();
		for (std::size_t i= 0; i < m_size; ++i)
			newData[i]= std::move(m_data[i]);
		delete[] m_data;
		m_data= newData;
		m_capacity= newCapacity;
	}

	T* m_data;
	std::size_t m_size;
	std::size_t m_capacity;
};
#pragma warning(pop)
}; // namespace Serialization

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableList_GENERATED
#endif
