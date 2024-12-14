#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableString.rfkh.h"
#endif

#include <string>

namespace Serialization NAMESPACE()
{
	class SERIALIZATION_API CLASS() String
	{
	public:
		String();
		String(const char* cstring);
		String(std::string const& string);
		String(std::string&& string);
		String(const String& other);
		String(String&& other) noexcept;
		virtual ~String();

		String& operator=(const char* other);
		String& operator=(const std::string& other);
		String& operator=(const String& other);

		void setValue(std::string const& string);
		const std::string& getValue() const;

		bool operator==(std::string const& other) const;
		bool operator!=(std::string const& other) const;
		bool operator==(String const& other) const;
		bool operator!=(String const& other) const;

		#ifdef SERIALIZATION_REFLECTION_ENABLED
		Serialization_String_GENERATED
		#endif

	private:
		struct StringData* m_pimpl;
	};
};

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableString_GENERATED
#endif