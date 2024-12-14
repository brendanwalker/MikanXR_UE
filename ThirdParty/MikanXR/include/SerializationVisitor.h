#pragma once

#include "SerializationExport.h"

#include <string>
#include "assert.h"

namespace rfk
{
	class Archetype;
	class Enum;
	class Field;
	class Struct;
	using Class = Struct;
	class Type;
};

namespace Serialization
{
	class SERIALIZATION_API ValueAccessor	
	{
	public:
		ValueAccessor(const void* instance, rfk::Field const& field);
		ValueAccessor(const void* instance, rfk::Type const& type);
		ValueAccessor(void* instance, rfk::Field const& field);
		ValueAccessor(void* instance, rfk::Type const& type);
		ValueAccessor(const ValueAccessor& other);
		ValueAccessor(ValueAccessor&& other);
		virtual ~ValueAccessor();

		ValueAccessor& operator=(const ValueAccessor& other);

		const void* getInstance() const;
		void* getInstanceMutable() const;

		rfk::Field const* getField() const;
		rfk::Type const& getType() const;
		std::string const& getName() const;

		rfk::Class const* getClassType() const;
		rfk::Struct const* getStructType() const;
		rfk::Enum const* getEnumType() const;

		const void* getUntypedValuePtr() const;
		void* getUntypedValueMutablePtr() const;

		template <typename t_value_type>
		const t_value_type& getTypedValueRef() const
		{
			return *getTypedValuePtr<t_value_type>();
		}

		template <typename t_value_type>
		t_value_type& getTypedValueMutableRef() const
		{
			return *getTypedValueMutablePtr<t_value_type>();
		}

		template <typename t_value_type>
		const t_value_type* getTypedValuePtr() const
		{
			assert(getType() == rfk::getType<t_value_type>());

			return reinterpret_cast<const t_value_type*>(getUntypedValuePtr());
		}

		template <typename t_value_type>
		t_value_type* getTypedValueMutablePtr() const
		{
			assert(getType() == rfk::getType<t_value_type>());

			return reinterpret_cast<t_value_type*>(getUntypedValueMutablePtr());
		}

		template <typename t_value_type>
		t_value_type getValue() const
		{
			assert(getType() == rfk::getType<t_value_type>());
			assert(getType().isValue());
			assert(getInstance() != nullptr);

			rfk::Field const* field= getField();
			if (field)
			{
				return field->getUnsafe<t_value_type>(getInstance());
			}
			else
			{
				return *reinterpret_cast<t_value_type*>(getInstance());
			}
		}

		template <typename t_value_type>
		void setValueByType(const t_value_type& value) const
		{
			assert(rfk::getType<t_value_type>().match(getType()));
			assert(getType().isValue());
			assert(getInstance() != nullptr);

			rfk::Field const* field= getField();
			if (field)
			{
				field->setUnsafe(getInstanceMutable(), &value, sizeof(t_value_type));
			}
			else
			{
				*reinterpret_cast<t_value_type*>(getInstanceMutable()) = value;
			}
		}

	private:
		struct AccessorData* m_pimpl;
	};

	class SERIALIZATION_API IVisitor
	{
	public:
		virtual void visitClass(ValueAccessor const& accessor) { }
		virtual void visitStruct(ValueAccessor const& accessor) { }
		virtual void visitEnum(ValueAccessor const& accessor) { }
		virtual void visitBool(ValueAccessor const& accessor) { }
		virtual void visitByte(ValueAccessor const& accessor) { }
		virtual void visitUByte(ValueAccessor const& accessor) { }
		virtual void visitShort(ValueAccessor const& accessor) { }
		virtual void visitUShort(ValueAccessor const& accessor) { }
		virtual void visitInt(ValueAccessor const& accessor) { }
		virtual void visitUInt(ValueAccessor const& accessor) { }
		virtual void visitLong(ValueAccessor const& accessor) { }
		virtual void visitULong(ValueAccessor const& accessor) { }
		virtual void visitFloat(ValueAccessor const& accessor) { }
		virtual void visitDouble(ValueAccessor const& accessor) { }
	};

	template <typename t_struct_type>
	void visitStruct(t_struct_type& instance, IVisitor *visitor)
	{
		visitStruct(&instance, t_struct_type::staticGetArchetype(), visitor, userdata);
	}

	template <typename t_struct_type>
	void visitStruct(const t_struct_type& instance, IVisitor* visitor)
	{
		visitStruct(&instance, t_struct_type::staticGetArchetype(), visitor, userdata);
	}

	SERIALIZATION_API void visitStruct(const void* instance, rfk::Struct const& structType, IVisitor *visitor);
	SERIALIZATION_API void visitStruct(void* instance, rfk::Struct const& structType, IVisitor *visitor);

	SERIALIZATION_API void visitField(const void* instance, rfk::Field const& fieldType, IVisitor *visitor);
	SERIALIZATION_API void visitField(void* instance, rfk::Field const& fieldType, IVisitor *visitor);

	SERIALIZATION_API void visitValue(ValueAccessor const& accessor, IVisitor *visitor);
};