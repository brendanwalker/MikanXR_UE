#pragma once

#include "MikanAPIExport.h"
#include "MikanTypeFwd.h"
#include "MikanCoreTypes.h"
#include "MikanVariantTypes.h"
#include "SerializableString.h"
#include "SerializableObjectPtr.h"

#ifdef MIKANAPI_REFLECTION_ENABLED
#include "MikanFunctionTypes.rfkh.h"
#endif // MIKANAPI_REFLECTION_ENABLED

#include <string>

/// Structure representing a component property value
struct MIKAN_API STRUCT(Serialization::CodeGenModule("MikanFunctionTypes")) MikanFunctionDescriptor
{
	FIELD() Serialization::String ownerSystemClass;
	FIELD() Serialization::String ownerComponentClass;
	FIELD() Serialization::String functionName;
	FIELD() Serialization::String displayName;

#ifdef MIKANAPI_REFLECTION_ENABLED
	MikanFunctionDescriptor_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED
};

#ifdef MIKANAPI_REFLECTION_ENABLED
File_MikanFunctionTypes_GENERATED
#endif // MIKANAPI_REFLECTION_ENABLED
