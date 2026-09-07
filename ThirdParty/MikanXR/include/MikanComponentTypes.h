#pragma once

#include "MikanAPIExport.h"
#include "MikanTypeFwd.h"
#include "MikanCoreTypes.h"
#include "SerializableString.h"
#include "SerializableObjectPtr.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanComponentTypes.rfkh.h"
#endif // MIKANAPI_REFLECTION_ENABLED

#include <string>

/// Polymorphic Structure representing TransformComponent values
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanComponentTypes")) MikanComponentValues
	: public Serialization::PolymorphicStruct
{
	FIELD() int component_id= INVALID_MIKAN_ID;
	FIELD() Serialization::String component_name;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanComponentValues_GENERATED
#endif
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanComponentTypes_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED
