#pragma once

#include "SerializationExport.h"
#include "SerializationVisitor.h"

#include <vector>

namespace Serialization
{
	template<typename t_object_type>
	bool serializeToBytes(const t_object_type& instance, std::vector<uint8_t>& outBytes)
	{
		return serializeToBytes(&instance, t_object_type::staticGetArchetype(), outBytes);
	}

	SERIALIZATION_API bool serializeToBytes(
		const void* instance, 
		rfk::Struct const& structType, 
		std::vector<uint8_t>& outBytes);
};

#pragma once