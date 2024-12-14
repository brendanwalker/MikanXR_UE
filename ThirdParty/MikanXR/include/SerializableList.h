#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableList.rfkh.h"
#endif

#include <vector>

namespace Serialization NAMESPACE()
{
	// Turns out that std::vector<bool> is a special case and doesn't behave like a normal std::vector
	// So we have to add this special case to handle it
	// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0174r0.html#3.1
	class SERIALIZATION_API CLASS() BoolList
	{
	public:
		BoolList();
		BoolList(const BoolList& other);
		BoolList(BoolList&& other) noexcept;
		virtual ~BoolList();

		BoolList& operator=(const BoolList& other);

		std::size_t size() const noexcept;
		void push_back(bool value);
		bool at(std::size_t index) const;
		void resize(std::size_t newSize);
		void clear();
		bool operator[](std::size_t index) const;

		const std::vector<bool>& getVector() const;
		std::vector<bool>& getVectorMutable();

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_BoolList_GENERATED
		#endif

	private:
		struct BoolListData* m_pimpl;
	};

	template <class T>
	class CLASS() List : public std::vector<T>
	{
	public:
		METHOD()
		std::size_t size() const noexcept
		{
			return std::vector<T>::size();
		}

		METHOD()
		void resize(const std::size_t& newSize) noexcept
		{
			std::vector<T>::resize(newSize);
		}

		METHOD()
		const void* getRawElement(const std::size_t& index) const
		{
			if (index >= 0 && index < size())
			{
				const T* data = std::vector<T>::data();
				const T* rawElement = data + index;

				return rawElement;
			}

			return nullptr;
		}

		METHOD()
		void* getRawElementMutable(const std::size_t& index)
		{
			return const_cast<void *>(getRawElement(index));
		}

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_List_GENERATED
		#endif
	};
};

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableList_GENERATED
#endif