#pragma once

#include "SerializationExport.h"
#include "SerializationVisitor.h"

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace Serialization
{
	template<typename t_object_type>
	bool serializeToJsonString(const t_object_type& instance, std::string& jsonString)
	{
		return serializeToJsonString(&instance, t_object_type::staticGetArchetype(), jsonString);
	}
	
	SERIALIZATION_API bool serializeToJsonString(
		const void* instance, rfk::Struct const& structType, std::string& jsonString);

	template<typename t_object_type>
	bool serializeToJson(const t_object_type& instance, nlohmann::json& jsonObject)
	{
		return serializeToJson(&instance, t_object_type::staticGetArchetype(), jsonObject);
	}

	SERIALIZATION_API bool serializeToJson(
		const void* instance, rfk::Struct const& structType, nlohmann::json& jsonObject);
};