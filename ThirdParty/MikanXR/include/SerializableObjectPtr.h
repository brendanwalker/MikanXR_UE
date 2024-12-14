#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableObjectPtr.rfkh.h"
#include "Refureku/Refureku.h"
#endif

#include <memory>

namespace rfk
{
	class Struct;
};

namespace Serialization NAMESPACE()
{
	template <class t_base_class>
	class CLASS() ObjectPtr
	{
	public:
		ObjectPtr()= default;

		// We can only assign a shared pointer to a ObjectPtr 
		// if we have access to serialization reflection structures
		// (needed for fetching the runtime class id)
		#ifdef SERIALIZATION_REFLECTION_ENABLED
		template <class t_derived_class>
		ObjectPtr(std::shared_ptr<t_derived_class>&objectPtr)
		{
			setSharedPointer(objectPtr);
		}

		template <class t_derived_class>
		void setSharedPointer(std::shared_ptr<t_derived_class>& objectPtr)
		{
			m_runtimeClassId = t_derived_class::staticGetArchetype().getId();
			m_objectPtr = std::move(objectPtr);
		}
		#endif

		std::shared_ptr<t_base_class> getSharedPointer() const
		{
			return m_objectPtr;
		}

		METHOD()
		std::size_t getRuntimeClassId() const
		{
			return m_runtimeClassId;
		}

		METHOD()
		const void* getRawPtr() const
		{
			return m_objectPtr.get();
		}

		// Only want this method defined when:
		// * When we have access to serialization reflection structures
		// * When we are generating the serialization reflection code
		#if defined(ENABLE_SERIALIZATION_REFLECTION) || defined(KODGEN_PARSING)
		METHOD()
		void* allocate(const std::size_t&& classId)
		{
			// If we are generating the serialization reflection code, we don't want to include Refureku
			// Just need the function signature to exist
			#ifndef KODGEN_PARSING
			rfk::Struct const* objectClass = rfk::getDatabase().getStructById(classId);

			if (objectClass != nullptr)
			{
				m_objectPtr= objectClass->makeSharedInstance<t_base_class>();
				m_runtimeClassId = classId;

				return m_objectPtr.get();
			}
			#endif

			return nullptr;
		}
		#endif

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_ObjectPtr_GENERATED
		#endif

	private:
		std::size_t m_runtimeClassId = 0;
		std::shared_ptr<t_base_class> m_objectPtr;
	};
};

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableObjectPtr_GENERATED
#endif