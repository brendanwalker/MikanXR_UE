#pragma once

#include "SerializationExport.h"

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include "SerializableString.rfkh.h"
#endif

namespace Serialization NAMESPACE()
{
// NOTE: This class intentionally exposes only `const char*` (never std::string) across its
// public interface. The std::string ABI (memory layout) differs between the Debug and Release
// CRT, so passing std::string by value/reference across a DLL boundary built against a different
// CRT (e.g. the Unreal plugin, which uses the Release CRT) corrupts the data. Keep all std::string
// usage hidden inside the .cpp pimpl and only marshal raw C strings here.
//
// ENCODING: The bytes are always treated as UTF-8 (this is what the JSON/binary serializers
// emit and consume). UTF-8 is NUL-safe and byte-oriented, so it round-trips losslessly through
// the const char* / internal std::string with no re-encoding. Convert any non-UTF-8 source
// (e.g. an active-code-page Windows string, or std::filesystem::path::string()) to UTF-8 at the
// ingestion boundary BEFORE storing it here. operator== is a byte-wise compare (no Unicode
// normalization), which is fine for identifiers/paths but not general text equality.
class SERIALIZATION_API CLASS() String
{
public:
	String();
	String(const char* cstring);
	String(const String& other);
	String(String&& other) noexcept;
	virtual ~String();

	String& operator=(const char* other);
	String& operator=(const String& other);

	void setUtf8Value(const char* str);
	const char* getUtf8Value() const;
	bool isEmpty() const;

	bool operator==(const char* other) const;
	bool operator!=(const char* other) const;
	bool operator==(String const& other) const;
	bool operator!=(String const& other) const;

#ifdef SERIALIZATION_REFLECTION_ENABLED
Serialization_String_GENERATED
#endif

	private : struct StringData* m_pimpl;
};
}; // namespace Serialization

#ifdef SERIALIZATION_REFLECTION_ENABLED
File_SerializableString_GENERATED
#endif