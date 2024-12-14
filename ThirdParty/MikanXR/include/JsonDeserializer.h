#pragma once

#include "SerializationExport.h"
#include "SerializationVisitor.h"

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace Serialization
{
	template<typename t_object_type>
	bool deserializeFromJsonString(const std::string& jsonString, t_object_type& instance)
	{
		return deserializeFromJsonString(jsonString, &instance, t_object_type::staticGetArchetype());
	}

	SERIALIZATION_API bool deserializeFromJsonString(
		const std::string& jsonString, 
		void* instance, 
		rfk::Struct const& structType);

	template<typename t_object_type>
	bool deserializeFromJson(const nlohmann::json& jsonObject, t_object_type& instance)
	{
		return deserializeFromJson(jsonObject, &instance, t_object_type::staticGetArchetype());
	}

	SERIALIZATION_API bool deserializeFromJson(
		const nlohmann::json& jsonObject, 
		void* instance, 
		rfk::Struct const& structType);
};