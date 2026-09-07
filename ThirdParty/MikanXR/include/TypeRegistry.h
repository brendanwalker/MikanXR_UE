#pragma once

#include "SerializationExport.h"
#include <string>

#ifdef SERIALIZATION_REFLECTION_ENABLED
#include <Refureku/TypeInfo/Archetypes/Struct.h>

namespace Serialization
{
class SERIALIZATION_API TypeRegistry
{
public:
	static void buildFromRfkDatabase();
	static rfk::Struct const* getStructByName(const std::string& typeName);
};
} // namespace Serialization
#endif // SERIALIZATION_REFLECTION_ENABLED
