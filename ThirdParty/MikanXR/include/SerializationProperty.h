#pragma once

#include "SerializationExport.h"

#if defined(ENABLE_SERIALIZATION_REFLECTION) || defined(KODGEN_PARSING)
	#include <Refureku/Properties/PropertySettings.h>
	#include <Refureku/TypeInfo/Archetypes/EnumValue.h>

	#ifndef KODGEN_PARSING
	#include "SerializationProperty.rfkh.h"
	#endif // !KODGEN_PARSING

	namespace rfk
	{
		class Enum;
		class EnumValue;
	}

	namespace Serialization NAMESPACE()
	{
		class SERIALIZATION_API CLASS(rfk::PropertySettings(rfk::EEntityKind::EnumValue, false, true)) 
			EnumStringValue : public rfk::Property
		{
		public:
			EnumStringValue() noexcept = default;
			EnumStringValue(char const* message) noexcept;
			virtual ~EnumStringValue();

			char const* getValue() const { return m_stringValue; }

			#ifndef KODGEN_PARSING
			Serialization_EnumStringValue_GENERATED
			#endif // !KODGEN_PARSING

		private:
			char* m_stringValue= nullptr;
		};

		class SERIALIZATION_API CLASS(rfk::PropertySettings(rfk::EEntityKind::Class | rfk::EEntityKind::Struct | rfk::EEntityKind::Enum, true, true)) 
			CodeGenModule : public rfk::Property
		{
		public:
			CodeGenModule() noexcept = default;
			CodeGenModule(char const* moduleName) noexcept;
			virtual ~CodeGenModule();

			char const* getModuleName() const { return m_moduleName; }

		#ifndef KODGEN_PARSING
			Serialization_CodeGenModule_GENERATED
			#endif // !KODGEN_PARSING

		private:
			char* m_moduleName = nullptr;
		};

		SERIALIZATION_API rfk::EnumValue const* findEnumValueByString(rfk::Enum const& enumType, char const* stringValue);
		SERIALIZATION_API char const* getEnumStringValue(rfk::EnumValue const& enumValue);
	}

	// Helper macros to assign a reflection properties 
	#define ENUMVALUE_STRING(enumString) ENUMVALUE(Serialization::EnumStringValue(enumString))

	#ifndef KODGEN_PARSING
	File_SerializationProperty_GENERATED
	#endif // !KODGEN_PARSING

#else

	#ifndef ENUMVALUE_STRING
	#define ENUMVALUE_STRING(...)
	#endif

#endif // ENABLE_SERIALIZATION_REFLECTION || KODGEN_PARSING